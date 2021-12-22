#include <catch2/catch_all.hpp>
#include <dualis.h>

using namespace dualis;
using namespace dualis::literals;

struct EachElementIs : Catch::Matchers::MatcherGenericBase
{
    EachElementIs(const std::byte value)
        : m_value{value}
    {
    }

    template <typename Range> bool match(Range const& other) const
    {
        for (auto const val : other)
        {
            if (val != m_value)
            {
                return false;
            }
        }
        return true;
    }

    std::string describe() const override
    {
        return "Each element is: 0x" + to_hex_string(m_value);
    }

private:
    std::byte m_value;
};

template <class ByteRange> struct EqualsByteRangeMatcher : Catch::Matchers::MatcherGenericBase
{
    EqualsByteRangeMatcher(const ByteRange& value, std::size_t count)
        : m_value{value}
        , m_count{count}
    {
    }

    template <typename OtherRange> bool match(OtherRange const& other) const
    {
        if (m_count == 0)
        {
            if (m_value.size() != other.size())
            {
                return false;
            }
            return std::memcmp(m_value.data(), other.data(), m_value.size()) == 0;
        }
        else
        {
            if (other.size() != m_count || m_value.size() < m_count)
            {
                return false;
            }
            return std::memcmp(m_value.data(), other.data(), m_count) == 0;
        }
    }

    std::string describe() const override
    {
        return "Equals";
    }

private:
    const ByteRange& m_value;
    std::size_t m_count{0};
};

struct NotModified : Catch::Matchers::MatcherGenericBase
{
    template <class Allocator> bool match(const Allocator& allocator) const
    {
        return allocator.stats->copied == 0 && allocator.stats->moved == 0 &&
               allocator.stats->deallocated == 0 && allocator.stats->allocated == 0;
    }

    std::string describe() const override
    {
        return "NotModified";
    }
};

template <class ByteRange>
auto EqualsByteRange(const ByteRange& bytes, const std::size_t count = 0)
    -> EqualsByteRangeMatcher<ByteRange>
{
    return EqualsByteRangeMatcher<ByteRange>{bytes, count};
}

struct allocator_stats
{
    std::size_t copied{0};
    std::size_t moved{0};
    std::size_t allocated{0};
    std::size_t deallocated{0};
    std::size_t selected{0};
};

template <class AlwaysEqual, class Propagate> struct mock_allocator
{
    using value_type = std::byte;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

    using is_always_equal = AlwaysEqual;
    using propagate_on_container_copy_assignment = Propagate;
    using propagate_on_container_move_assignment = Propagate;
    using propagate_on_container_swap = Propagate;

    std::allocator<std::byte> allocator;
    allocator_stats* stats;
    unsigned id{0};

    constexpr mock_allocator(allocator_stats* stats, unsigned id = 0)
        : stats{stats}
        , id{id}
    {
    }

    constexpr mock_allocator(const mock_allocator& other) noexcept
        : allocator{other.allocator}
        , stats{other.stats}
        , id{other.id}
    {
        stats->copied += 1;
    }

    constexpr mock_allocator(mock_allocator&& other) noexcept
        : allocator{std::move(other.allocator)}
        , stats{other.stats}
        , id{other.id}
    {
        stats->moved += 1;
    }

    constexpr auto operator=(const mock_allocator& other) noexcept -> mock_allocator&
    {
        if (this != std::addressof(other))
        {
            stats = other.stats;
            id = other.id;
            stats->copied += 1;
        }
        return *this;
    }

    constexpr auto operator=(mock_allocator&& other) noexcept -> mock_allocator&
    {
        if (this != std::addressof(other))
        {
            stats = other.stats;
            id = other.id;
            stats->moved += 1;
        }
        return *this;
    }

    [[nodiscard]] constexpr std::byte* allocate(std::size_t n)
    {
        stats->allocated += 1;
        return allocator.allocate(n);
    }

    constexpr void deallocate(std::byte* p, std::size_t n)
    {
        stats->deallocated += 1;
        allocator.deallocate(p, n);
    }

    [[nodiscard]] constexpr auto select_on_container_copy_construction() const -> mock_allocator
    {
        auto new_alloc = *this;
        new_alloc.stats->selected += 1;
        return new_alloc;
    }

    [[nodiscard]] constexpr bool operator==(const mock_allocator& rhs) const noexcept
    {
        if (is_always_equal::value)
        {
            return true;
        }
        else
        {
            return id == rhs.id;
        }
    }

    void start_tracking()
    {
        *stats = {0};
    }
};

using always_equal_allocator = mock_allocator<std::true_type, std::false_type>;
using not_always_equal_allocator = mock_allocator<std::false_type, std::true_type>;
using propagate_allocator = mock_allocator<std::false_type, std::true_type>;
using dont_propagate_allocator = always_equal_allocator;
using propagate_always_equal_allocator = mock_allocator<std::true_type, std::true_type>;

template <class Allocator, std::size_t EmbeddedSize>
auto make_allocated_byte_storage(allocator_stats* stats, unsigned id = 0)
    -> dualis::detail::_byte_storage<Allocator, EmbeddedSize>
{
    return dualis::detail::_byte_storage<Allocator, EmbeddedSize>{EmbeddedSize + 1,
                                                                  Allocator{stats, id}};
}

template <class Allocator, std::size_t EmbeddedSize>
auto make_embedded_byte_storage(allocator_stats* stats, unsigned id = 0)
    -> dualis::detail::_byte_storage<Allocator, EmbeddedSize>
{
    return dualis::detail::_byte_storage<Allocator, EmbeddedSize>{EmbeddedSize - 1,
                                                                  Allocator{stats, id}};
}

#define CLASS_UNDER_TEST "_byte_storage"

SCENARIO(CLASS_UNDER_TEST ": assignment", "[containers]")
{
    static constexpr std::size_t EmbeddedSize = 16;
    static constexpr std::size_t EmbeddedStorageSize = EmbeddedSize - 1;
    static constexpr std::size_t AllocatedStorageSize = EmbeddedSize + 1;
    static constexpr std::size_t DefaultId = 0;
    static constexpr std::size_t DistinctId = 1;
    using namespace dualis::detail;
    std::array<std::byte, EmbeddedSize> TestData{0x10_b, 0x11_b, 0x12_b, 0x13_b, 0x10_b, 0x11_b,
                                                 0x12_b, 0x13_b, 0x10_b, 0x11_b, 0x12_b, 0x13_b,
                                                 0x10_b, 0x11_b, 0x12_b, 0x13_b};

    allocator_stats stats;
    allocator_stats other_stats;

    GIVEN(CLASS_UNDER_TEST " _bytes_; // constructed")
    {
        WHEN("_bytes_.reassign(count); // discard contents and resize")
        {
            using byte_storage = _byte_storage<always_equal_allocator, EmbeddedSize>;

            AND_WHEN("count <= _bytes_.capacity()")
            {
                auto _bytes_ =
                    make_embedded_byte_storage<always_equal_allocator, EmbeddedSize>(&stats);
                auto* old_data = _bytes_.data();
                auto const old_capacity = _bytes_.capacity();

                auto* data = _bytes_.reassign(_bytes_.capacity() - 1);

                THEN("the data pointer remains unchanged")
                {
                    REQUIRE(_bytes_.data() == old_data);
                }
                THEN("the capacity remains unchanged")
                {
                    REQUIRE(_bytes_.capacity() == old_capacity);
                }
                THEN("the return value equals the data pointer")
                {
                    REQUIRE(data == _bytes_.data());
                }
            }
            AND_WHEN("count > _bytes_.capacity()")
            {
                AND_WHEN("_bytes_.is_allocated()")
                {
                    auto _bytes_ =
                        make_allocated_byte_storage<always_equal_allocator, EmbeddedSize>(&stats);
                    _bytes_.get_allocator().start_tracking();

                    auto const count = _bytes_.capacity() + 1;
                    auto* data = _bytes_.reassign(count);

                    THEN("the old data is deallocated")
                    {
                        REQUIRE(stats.deallocated == 1);
                    }
                    THEN("the new data is allocated")
                    {
                        REQUIRE(stats.allocated == 1);
                    }
                    THEN("the new capacity is at least count")
                    {
                        REQUIRE(_bytes_.capacity() >= count);
                    }
                    THEN("the return value equals the data pointer")
                    {
                        REQUIRE(data == _bytes_.data());
                    }
                }
                AND_WHEN("!_bytes_.is_allocated()")
                {
                    auto _bytes_ =
                        make_embedded_byte_storage<always_equal_allocator, EmbeddedSize>(&stats);
                    _bytes_.get_allocator().start_tracking();

                    auto const count = _bytes_.capacity() + 1;
                    auto* data = _bytes_.reassign(count);

                    THEN("no deallocation takes place")
                    {
                        REQUIRE(stats.deallocated == 0);
                    }
                    THEN("the new data is allocated")
                    {
                        REQUIRE(stats.allocated == 1);
                    }
                    THEN("the new capacity is at least count")
                    {
                        REQUIRE(_bytes_.capacity() >= count);
                    }
                    THEN("the return value equals the data pointer")
                    {
                        REQUIRE(data == _bytes_.data());
                    }
                }
            }
        }

        WHEN("_bytes_.assign(bytes_ptr, count); // assign array of bytes")
        {
            auto _bytes_ = make_embedded_byte_storage<always_equal_allocator, EmbeddedSize>(&stats);

            _bytes_.assign(TestData.data(), _bytes_.size());

            THEN("_bytes_ equals _bytes_ptr")
            {
                REQUIRE_THAT(_bytes_, EqualsByteRange(TestData, _bytes_.size()));
            }
        }

        WHEN("_bytes_.change_allocator(new_alloc, destroy); // assign new allocator")
        {
            THEN("_bytes_ has new_alloc")
            {
                static constexpr bool DoesntMatter = true;
                auto _bytes_ = make_embedded_byte_storage<always_equal_allocator, EmbeddedSize>(
                    &stats, DefaultId);
                decltype(_bytes_)::allocator_type new_alloc{&other_stats, DistinctId};

                new_alloc.start_tracking();
                _bytes_.change_allocator(new_alloc, DoesntMatter);

                REQUIRE(_bytes_.get_allocator().id == DistinctId);

                AND_THEN("it is a copy")
                {
                    REQUIRE(new_alloc.stats->copied == 1);
                }
            }

            AND_WHEN("_bytes_.is_allocated()")
            {
                AND_WHEN("allocator_traits::is_always_equal")
                {
                    auto _bytes_ =
                        make_allocated_byte_storage<always_equal_allocator, EmbeddedSize>(
                            &stats, DefaultId);
                    static_assert(decltype(_bytes_)::allocator_traits::is_always_equal::value);
                    decltype(_bytes_)::allocator_type new_alloc{&other_stats, DistinctId};

                    AND_WHEN("destroy")
                    {
                        _bytes_.change_allocator(new_alloc, true);

                        THEN("its data is deallocated")
                        {
                            REQUIRE(stats.deallocated == 1);
                        }
                    }
                    AND_WHEN("!destroy")
                    {
                        _bytes_.change_allocator(new_alloc, false);

                        THEN("its data is not deallocated")
                        {
                            REQUIRE(stats.deallocated == 0);
                        }
                    }
                }
                AND_WHEN("!allocator_traits::is_always_equal")
                {
                    auto _bytes_ =
                        make_allocated_byte_storage<not_always_equal_allocator, EmbeddedSize>(
                            &stats, DefaultId);
                    static_assert(!decltype(_bytes_)::allocator_traits::is_always_equal::value);

                    AND_WHEN("_bytes_.allocator() != new_alloc")
                    {
                        decltype(_bytes_)::allocator_type new_alloc{&other_stats, DistinctId};
                        REQUIRE(_bytes_.get_allocator() != new_alloc);

                        _bytes_.change_allocator(new_alloc, false);

                        THEN("its data is deallocated")
                        {
                            REQUIRE(stats.deallocated == 1);
                        }
                    }
                    AND_WHEN("_bytes_.allocator() == new_alloc")
                    {
                        decltype(_bytes_)::allocator_type new_alloc{&other_stats, DefaultId};
                        REQUIRE(_bytes_.get_allocator() == new_alloc);

                        _bytes_.change_allocator(new_alloc, false);

                        THEN("its data is not deallocated")
                        {
                            REQUIRE(stats.deallocated == 0);
                        }
                    }
                }
            }
        }

        WHEN("_bytes_.assign(_other_); // copy-assign")
        {
            AND_WHEN("!allocator_traits::propagate_on_container_copy_assignment")
            {
                auto _bytes_ =
                    make_embedded_byte_storage<dont_propagate_allocator, EmbeddedSize>(&stats);
                auto _other_ = make_embedded_byte_storage<dont_propagate_allocator, EmbeddedSize>(
                    &other_stats, DistinctId);
                static_assert(!decltype(_bytes_)::allocator_traits::
                                  propagate_on_container_copy_assignment::value);
                std::memcpy(_other_.data(), TestData.data(), _other_.size());
                _bytes_.get_allocator().start_tracking();
                _other_.get_allocator().start_tracking();

                _bytes_.assign(_other_);

                THEN("the allocator of _bytes_ is not modified")
                {
                    REQUIRE_THAT(_bytes_.get_allocator(), NotModified());
                    REQUIRE(_bytes_.get_allocator().id == DefaultId);
                }
                THEN("the allocator of _other_ is not modified")
                {
                    REQUIRE_THAT(_other_.get_allocator(), NotModified());
                    REQUIRE(_other_.get_allocator().id == DistinctId);
                }
            }
            AND_WHEN("allocator_traits::propagate_on_container_copy_assignment")
            {
                auto _bytes_ =
                    make_embedded_byte_storage<propagate_allocator, EmbeddedSize>(&stats);
                auto _other_ = make_embedded_byte_storage<propagate_allocator, EmbeddedSize>(
                    &other_stats, DistinctId);
                static_assert(decltype(_bytes_)::allocator_traits::
                                  propagate_on_container_copy_assignment::value);
                std::memcpy(_other_.data(), TestData.data(), _other_.size());
                _bytes_.get_allocator().start_tracking();
                _other_.get_allocator().start_tracking();

                _bytes_.assign(_other_);

                THEN("the allocator of _other_ has been propagated to _bytes_")
                {
                    REQUIRE(_bytes_.get_allocator().id == _other_.get_allocator().id);

                    THEN("by copying")
                    {
                        REQUIRE(other_stats.copied == 1);
                    }
                }
            }
        }
    }
}

/*


template <class ByteRange> void requireSizeN(ByteRange& bytes, const std::size_t N)
{
    THEN("its size is N")
    {
        REQUIRE(bytes.size() == N);
    }
    THEN("it is not considered empty")
    {
        REQUIRE_FALSE(bytes.empty());
    }
    THEN("its capacity is at least N")
    {
        REQUIRE(bytes.capacity() >= N);
    }
    THEN("its begin and end iterators are N apart")
    {
        REQUIRE(bytes.begin() + N == bytes.end());
        auto const& const_bytes = bytes;
        REQUIRE(const_bytes.begin() + N == const_bytes.end());
        REQUIRE(bytes.cbegin() + N == bytes.cend());
    }
}

template <class T, class U> bool stored_within(const T* a, const U* b)
{
    auto const* begin = reinterpret_cast<const void*>(b);
    auto const* end = reinterpret_cast<const void*>(b + 1);
    auto const* a_void = reinterpret_cast<const void*>(a);
    return a_void >= begin && a_void < end;
}

struct allocator_stats
{
    std::size_t copied{0};
    std::size_t moved{0};
    std::size_t allocated{0};
    std::size_t deallocated{0};
    std::size_t selected{0};
};

struct mock_allocator
{
    using value_type = std::byte;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

    std::allocator<std::byte> allocator;
    allocator_stats* stats;
    unsigned id{0};

    constexpr mock_allocator(allocator_stats* stats, unsigned id = 0)
        : stats{stats}
        , id{id}
    {
    }

    constexpr mock_allocator(const mock_allocator& other) noexcept
        : allocator{other.allocator}
        , stats{other.stats}
        , id{other.id}
    {
        stats->copied += 1;
    }

    constexpr mock_allocator(mock_allocator&& other) noexcept
        : allocator{std::move(other.allocator)}
        , stats{other.stats}
        , id{other.id}
    {
        stats->moved += 1;
    }

    constexpr auto operator=(const mock_allocator& other) noexcept -> mock_allocator&
    {
        if (this != std::addressof(other))
        {
            stats = other.stats;
            id = other.id;
            stats->copied += 1;
        }
        return *this;
    }

    constexpr auto operator=(mock_allocator&& other) noexcept -> mock_allocator&
    {
        if (this != std::addressof(other))
        {
            stats = other.stats;
            id = other.id;
            stats->moved += 1;
        }
        return *this;
    }

    [[nodiscard]] constexpr std::byte* allocate(std::size_t n)
    {
        stats->allocated += 1;
        return allocator.allocate(n);
    }

    constexpr void deallocate(std::byte* p, std::size_t n)
    {
        stats->deallocated += 1;
        allocator.deallocate(p, n);
    }

    [[nodiscard]] constexpr auto select_on_container_copy_construction() const -> mock_allocator
    {
        auto new_alloc = *this;
        new_alloc.stats->selected += 1;
        return new_alloc;
    }

    [[nodiscard]] constexpr bool operator==(const mock_allocator& rhs) const noexcept
    {
        return id == rhs.id;
    }
};

template <class PropagateOnCopy, class PropagateOnMove>
struct propagate_mock_allocator : public mock_allocator
{
    using propagate_on_container_copy_assignment = PropagateOnCopy;
    using propagate_on_container_move_assignment = PropagateOnMove;
    using mock_allocator::mock_allocator;
};

#define CLASS_UNDER_TEST "_byte_storage"

SCENARIO(CLASS_UNDER_TEST " [LocalSize > 0]: construction and destruction",
         "[detail::" CLASS_UNDER_TEST "][containers]")
{
    using namespace dualis::detail;
    static constexpr std::size_t LocalSize = 16;
    allocator_stats stats;
    allocator_stats other_stats;

    //== construction from allocator ==============================================================

    GIVEN("An allocator alloc")
    {
        mock_allocator alloc{&stats};
        WHEN("constructing using cut = " CLASS_UNDER_TEST "{alloc}")
        {
            _byte_storage<mock_allocator, LocalSize> cut{alloc};
            THEN("its size is 0")
            {
                REQUIRE(cut.size() == 0);
            }
            THEN("its capacity is LocalSize")
            {
                REQUIRE(cut.capacity() == LocalSize);
            }
            THEN("data() is stored within cut")
            {
                REQUIRE(stored_within(cut.data(), &cut));
            }
            THEN("no allocation has taken place")
            {
                REQUIRE(stats.allocated == 0);
            }
        }
        WHEN("constructing using cut = " CLASS_UNDER_TEST "{size, alloc}")
        {
            AND_WHEN("size > LocalSize")
            {
                static constexpr auto size = LocalSize + 1;
                _byte_storage<mock_allocator, LocalSize> cut{size, alloc};

                THEN("data() is allocated")
                {
                    REQUIRE(cut.is_allocated());
                }
                THEN("1 allocation has taken place")
                {
                    REQUIRE(stats.allocated == 1);
                }
            }
            AND_WHEN("size == LocalSize")
            {
                static constexpr auto size = LocalSize;
                _byte_storage<mock_allocator, LocalSize> cut{size, alloc};

                THEN("data() is not allocated")
                {
                    REQUIRE_FALSE(cut.is_allocated());
                }
            }
            AND_WHEN("size < LocalSize")
            {
                static constexpr auto size = LocalSize - 1;
                _byte_storage<mock_allocator, LocalSize> cut{size, alloc};

                THEN("data() is not allocated")
                {
                    REQUIRE_FALSE(cut.is_allocated());
                }
            }
        }
    }

    GIVEN(CLASS_UNDER_TEST " other{size, alloc};")
    {
        WHEN("other is not allocated (size <= LocalSize)")
        {
            _byte_storage<mock_allocator, LocalSize> other{8, mock_allocator{&stats}};
            REQUIRE_FALSE(other.is_allocated());

            std::memset(other.data(), 12, other.size());

            AND_WHEN(CLASS_UNDER_TEST " cut{other}; // copy-constructing without new allocator")
            {
                auto old_stats = stats;
                _byte_storage<mock_allocator, LocalSize> cut{other};
                THEN("the allocator is copied using alloc.select_on_container_copy_construction()")
                {
                    REQUIRE(stats.selected == old_stats.selected + 1);
                }
                THEN("copy is not allocated")
                {
                    REQUIRE_FALSE(cut.is_allocated());

                    AND_THEN("allocation has not taken place")
                    {
                        REQUIRE(stats.allocated == old_stats.allocated);
                    }
                }
                THEN("other and copy have the same size")
                {
                    REQUIRE(other.size() == cut.size());

                    AND_THEN("both have the same contents")
                    {
                        REQUIRE(std::memcmp(cut.data(), other.data(), cut.size()) == 0);
                    }
                }
                THEN("other and copy have the same capacity")
                {
                    REQUIRE(other.capacity() == cut.capacity());
                }
            }
        }
        WHEN("other is allocated (size > LocalSize)")
        {
            _byte_storage<mock_allocator, LocalSize> other{LocalSize + 1, mock_allocator{&stats}};
            REQUIRE(other.is_allocated());

            AND_WHEN(CLASS_UNDER_TEST " cut{other}; // copy-constructing without new allocator")
            {
                auto const old_stats = stats;
                _byte_storage<mock_allocator, LocalSize> cut{other};
                THEN("copy is allocated")
                {
                    REQUIRE(cut.is_allocated());

                    AND_THEN("allocation has taken place")
                    {
                        REQUIRE(stats.allocated == old_stats.allocated + 1);
                    }
                }
                THEN("copy and other have different data pointers")
                {
                    REQUIRE(cut.data() != other.data());
                }
            }
        }
    }

    //== copy/move constructors ===================================================================

    GIVEN(CLASS_UNDER_TEST " other{size, alloc};")
    {
        allocator_stats other_stats;

        WHEN(CLASS_UNDER_TEST
             " copied{other, other_alloc}; // copy-construction with new allocator")
        {
            AND_WHEN("!other.is_allocated() // data is in embedded storage")
            {
                _byte_storage<mock_allocator, LocalSize> other{8, mock_allocator{&stats}};
                allocator_stats new_stats;
                _byte_storage<mock_allocator, LocalSize> copied{other, mock_allocator{&new_stats}};

                THEN("the allocator of other is not copied")
                {
                    REQUIRE(stats.copied == 1);
                }
                THEN("no allocation took place")
                {
                    REQUIRE(new_stats.allocated == 0);
                }
            }
        }

        WHEN(CLASS_UNDER_TEST
             " moved{std::move(other), other_alloc}; // move-construction with new allocator")
        {
            AND_WHEN("other.is_allocated() // data is in allocated storage")
            {
                AND_WHEN("alloc == other_alloc // storage can be (de)allocated interchangeably")
                {
                    _byte_storage<mock_allocator, LocalSize> other{LocalSize + 1,
                                                                   mock_allocator{&stats, 0}};
                    allocator_stats new_stats;
                    auto const* old_data = other.data();
                    auto const old_capacity = other.capacity();
                    _byte_storage<mock_allocator, LocalSize> copied{std::move(other),
                                                                    mock_allocator{&new_stats, 0}};

                    THEN("the allocator of other is not copied")
                    {
                        REQUIRE(new_stats.copied == stats.copied);
                        REQUIRE(copied.get_allocator().stats == &new_stats);
                        REQUIRE(other.get_allocator().stats == &stats);
                    }
                    THEN("no allocation took place")
                    {
                        REQUIRE(new_stats.allocated == 0);
                    }
                    THEN("no deallocation took place")
                    {
                        REQUIRE(stats.deallocated == 0);
                    }
                    THEN("moved is allocated")
                    {
                        REQUIRE(copied.is_allocated());

                        AND_THEN("the data pointer is moved over")
                        {
                            REQUIRE(copied.data() == old_data);
                        }
                        AND_THEN("the capacity is moved over")
                        {
                            REQUIRE(copied.capacity() == old_capacity);
                        }
                    }
                    THEN("other is not allocated anymore")
                    {
                        REQUIRE_FALSE(other.is_allocated());

                        AND_THEN("the capacity of other is LocalSize")
                        {
                            REQUIRE(other.capacity() == LocalSize);
                        }
                    }
                    THEN("the size of other is 0")
                    {
                        REQUIRE(other.size() == 0);
                    }
                }

                AND_WHEN("alloc != other_alloc // storage cannot be (de)allocated interchangeably")
                {
                    _byte_storage<mock_allocator, LocalSize> other{LocalSize + 1,
                                                                   mock_allocator{&stats, 0}};
                    allocator_stats new_stats;
                    auto const* old_data = other.data();
                    auto const old_capacity = other.capacity();
                    std::byte old_contents[LocalSize + 1];
                    std::memcpy(old_contents, other.data(), other.size());
                    _byte_storage<mock_allocator, LocalSize> copied{std::move(other),
                                                                    mock_allocator{&new_stats, 1}};

                    THEN("the allocator of other is not copied")
                    {
                        REQUIRE(new_stats.copied == stats.copied);
                        REQUIRE(copied.get_allocator().stats == &new_stats);
                        REQUIRE(other.get_allocator().stats == &stats);
                    }
                    THEN("one allocation took place")
                    {
                        REQUIRE(new_stats.allocated == 1);
                    }
                    THEN("one deallocation took place")
                    {
                        REQUIRE(stats.deallocated == 1);
                    }
                    THEN("moved has its data in allocated storage")
                    {
                        REQUIRE(copied.is_allocated());

                        AND_THEN("the data is copied over")
                        {
                            REQUIRE(std::memcmp(copied.data(), old_contents, copied.size()) == 0);
                        }
                        AND_THEN("the capacity is moved over")
                        {
                            REQUIRE(copied.capacity() == old_capacity);
                        }
                    }
                    THEN("other is not allocated anymore")
                    {
                        REQUIRE_FALSE(other.is_allocated());

                        AND_THEN("the capacity of other is LocalSize")
                        {
                            REQUIRE(other.capacity() == LocalSize);
                        }
                    }
                    THEN("the size of other is 0")
                    {
                        REQUIRE(other.size() == 0);
                    }
                }
            }
            AND_WHEN("!other.is_allocated() // data is in embedded storage")
            {
                _byte_storage<mock_allocator, LocalSize> other{LocalSize - 1,
                                                               mock_allocator{&stats, 0}};
                REQUIRE_FALSE(other.is_allocated());

                allocator_stats new_stats;
                std::byte old_contents[LocalSize - 1];
                auto const old_size = other.size();
                std::memcpy(old_contents, other.data(), other.size());
                _byte_storage<mock_allocator, LocalSize> moved{std::move(other),
                                                               mock_allocator{&new_stats, 1}};

                THEN("the data of moved is not in allocated storage")
                {
                    REQUIRE_FALSE(moved.is_allocated());

                    AND_THEN("the capacity of moved is LocalSize")
                    {
                        REQUIRE(moved.capacity() == LocalSize);
                    }
                }
                THEN("the size of moved is the same as other's was")
                {
                    REQUIRE(moved.size() == old_size);

                    AND_THEN("the data of other is copied to moved")
                    {
                        REQUIRE(std::memcmp(moved.data(), old_contents, moved.size()) == 0);
                    }
                }
            }
        }

        WHEN(CLASS_UNDER_TEST
             " moved{std::move(other)}; // move-construction without new allocator")
        {
            AND_WHEN("!other.is_allocated() // data is in embedded storage")
            {
                _byte_storage<mock_allocator, LocalSize> other{8, mock_allocator{&other_stats}};
                REQUIRE_FALSE(other.is_allocated());
                std::memset(other.data(), 12, other.size());

                auto const old_moved = other_stats.moved;
                auto const old_copied = other_stats.copied;
                auto const* old_data = other.data();
                auto const old_capacity = other.capacity();
                auto const old_size = other.size();
                _byte_storage<mock_allocator, LocalSize> moved{std::move(other)};

                THEN("the allocator is moved")
                {
                    REQUIRE(other_stats.moved == old_moved + 1);
                }
                THEN("the allocator is not copied")
                {
                    REQUIRE(other_stats.copied == old_copied);
                }
                THEN("other.data() is unchanged")
                {
                    REQUIRE(other.data() == old_data);
                }
                THEN("other.capacity() is unchanged")
                {
                    REQUIRE(other.capacity() == old_capacity);
                }
                THEN("other.size() is 0")
                {
                    REQUIRE(other.size() == 0);
                }
                THEN("moved has the same size as other had")
                {
                    REQUIRE(moved.size() == old_size);
                }
                THEN("the data has been copied")
                {
                    REQUIRE(std::memcmp(moved.data(), other.data(), moved.size()) == 0);
                }
            }
            AND_WHEN("other.is_allocated() // data is in allocated storage")
            {
                _byte_storage<mock_allocator, LocalSize> other{LocalSize + 1,
                                                               mock_allocator{&other_stats}};
                REQUIRE(other.is_allocated());
                std::memset(other.data(), 12, other.size());

                auto const old_moved = other_stats.moved;
                auto const old_copied = other_stats.copied;
                auto const* old_data = other.data();
                auto const old_capacity = other.capacity();
                _byte_storage<mock_allocator, LocalSize> moved{std::move(other)};

                THEN("other.data() is stored within other")
                {
                    REQUIRE(stored_within(other.data(), &other));
                }
                THEN("other.capacity() is LocalSize")
                {
                    REQUIRE(other.capacity() == LocalSize);
                }
                THEN("other.size() is 0")
                {
                    REQUIRE(other.size() == 0);
                }
                THEN("moved takes the data pointer from other")
                {
                    REQUIRE(moved.data() == old_data);
                }
            }
        }
    }

    //== destructor ===============================================================================

    GIVEN("An instance cut of " CLASS_UNDER_TEST)
    {
        WHEN("~" CLASS_UNDER_TEST "(); // it is destroyed")
        {
            AND_WHEN("!cut.is_allocated()")
            {
                {
                    _byte_storage<mock_allocator, LocalSize> cut{mock_allocator{&stats}};
                }
                THEN("no delloaction takes place")
                {
                    REQUIRE(stats.deallocated == 0);
                }
            }
            AND_WHEN("cut.is_allocated()")
            {
                {
                    _byte_storage<mock_allocator, LocalSize> cut{LocalSize + 1,
                                                                 mock_allocator{&stats}};
                }
                THEN("delloaction takes place")
                {
                    REQUIRE(stats.deallocated == 1);
                }
            }
        }
    }
}

SCENARIO(CLASS_UNDER_TEST " [LocalSize == 0]: construction",
         "[detail::" CLASS_UNDER_TEST "][containers]")
{
    using namespace dualis::detail;

    GIVEN("An allocator alloc")
    {
        std::allocator<std::byte> alloc;
        WHEN("constructing using " CLASS_UNDER_TEST "{alloc}")
        {
            _byte_storage<std::allocator<std::byte>, 0> cut{alloc};
            THEN("its size is 0")
            {
                REQUIRE(cut.size() == 0);
            }
            THEN("its capacity is 0 (nothing allocated)")
            {
                REQUIRE(cut.capacity() == 0);
            }
            THEN("data() == nullptr (nothing allocated)")
            {
                REQUIRE(cut.data() == nullptr);
            }
        }
    }
}

SCENARIO(CLASS_UNDER_TEST ": assignment", "[containers]")
{
    static constexpr std::size_t EmbeddedSize = 16;
    using namespace dualis::detail;
    std::array<std::byte, EmbeddedSize> TestData{0x10_b, 0x11_b, 0x12_b, 0x13_b, 0x10_b, 0x11_b,
                                                 0x12_b, 0x13_b, 0x10_b, 0x11_b, 0x12_b, 0x13_b,
                                                 0x10_b, 0x11_b, 0x12_b, 0x13_b};

    GIVEN(CLASS_UNDER_TEST " _bytes_; // constructed")
    {
        WHEN("_bytes_.assign(_other_); // copy-assign")
        {
            AND_WHEN("!Allocator::propagate_on_container_copy_assignment")
            {
                using byte_storage =
                    _byte_storage<propagate_mock_allocator<std::false_type, std::false_type>,
                                  EmbeddedSize>;
                allocator_stats stats;
                propagate_mock_allocator<std::false_type, std::false_type> alloc{&stats};
                allocator_stats other_stats;
                propagate_mock_allocator<std::false_type, std::false_type> other_alloc{
                    &other_stats};

                byte_storage _bytes_{EmbeddedSize - 1, alloc};
                byte_storage _other_{EmbeddedSize - 1, other_alloc};
                std::memcpy(_other_.data(), TestData.data(), _other_.size());

                _bytes_.assign(_other_);

                THEN("both have the same size")
                {
                    REQUIRE(_bytes_.size() == _other_.size());

                    AND_THEN("both have the same contents")
                    {
                        REQUIRE_THAT(_bytes_, EqualsByteRange(_other_));
                    }
                }
                THEN("both allocators weren't touched")
                {
                    REQUIRE(_bytes_.get_allocator().stats == &stats);
                    REQUIRE(_other_.get_allocator().stats == &other_stats);
                }
            }
            AND_WHEN("Allocator::propagate_on_container_copy_assignment")
            {
                using byte_storage =
                    _byte_storage<propagate_mock_allocator<std::true_type, std::true_type>,
                                  EmbeddedSize>;
                allocator_stats stats;
                propagate_mock_allocator<std::true_type, std::true_type> alloc{&stats};
                allocator_stats other_stats;
                propagate_mock_allocator<std::true_type, std::true_type> other_alloc{&other_stats};

                AND_WHEN("_other_.size() <= _bytes_.capacity()")
                {
                    byte_storage _bytes_{EmbeddedSize - 1, alloc};
                    byte_storage _other_{EmbeddedSize - 1, other_alloc};
                    REQUIRE(_other_.size() <= _bytes_.capacity());
                    std::memcpy(_other_.data(), TestData.data(), _other_.size());

                    other_stats = {0};
                    _bytes_.assign(_other_);

                    THEN("both have the same size")
                    {
                        REQUIRE(_bytes_.size() == _other_.size());

                        AND_THEN("both have the same contents")
                        {
                            REQUIRE_THAT(_bytes_, EqualsByteRange(_other_));
                        }
                    }
                    THEN("allocator of _other_ has been propagated to _bytes_")
                    {
                        REQUIRE(_bytes_.get_allocator().stats == &other_stats);

                        AND_THEN("by copying")
                        {
                            REQUIRE(other_stats.copied == 1);
                        }
                    }
                }
                AND_WHEN("_other_.size() > _bytes_.capacity()")
                {
                    byte_storage _other_{EmbeddedSize + 2, other_alloc};
                    std::memcpy(_other_.data(), TestData.data(),
                                std::min(TestData.size(), _other_.size()));

                    AND_WHEN("!_bytes_.is_allocated()")
                    {
                        byte_storage _bytes_{EmbeddedSize - 1, alloc};
                        REQUIRE_FALSE(_bytes_.is_allocated());
                        REQUIRE(_other_.size() > _bytes_.capacity());

                        _bytes_.assign(_other_);

                        THEN("both have the same size")
                        {
                            REQUIRE(_bytes_.size() == _other_.size());

                            AND_THEN("both have the same contents")
                            {
                                REQUIRE_THAT(_bytes_, EqualsByteRange(_other_));
                            }
                        }
                        THEN("allocator of _other_ has been propagated to _bytes_")
                        {
                            REQUIRE(_bytes_.get_allocator().stats == &other_stats);
                        }
                    }
                    AND_WHEN("_bytes_.is_allocated()")
                    {
                        byte_storage _bytes_{EmbeddedSize + 1, alloc};
                        REQUIRE(_bytes_.is_allocated());
                        REQUIRE(_other_.size() > _bytes_.capacity());

                        _bytes_.assign(_other_);

                        THEN("the data of _bytes_ has been deallocated")
                        {
                            REQUIRE(stats.deallocated == 1);
                        }
                    }
                }
            }
        }
    }
}

/*

SCENARIO(CLASS_UNDER_TEST ": element access", "[small_byte_vector][containers]")
{
GIVEN("A non-empty " CLASS_UNDER_TEST)
{
    small_byte_vector bytes{0x13_b, 0x14_b, 0x15_b};

    WHEN("value = operator[](i) with i < size()")
    {
        THEN("value is the contained value")
        {
            REQUIRE(bytes[0] == 0x13_b);
        }
    }
    WHEN("operator[](i) = value with i < size()")
    {
        bytes[0] = 0x17_b;
        THEN("the contained value is modified")
        {
            REQUIRE(bytes[0] == 0x17_b);
        }
    }
    WHEN("value = front()")
    {
        THEN("value is the first value")
        {
            REQUIRE(bytes.front() == bytes[0]);
        }
    }
    WHEN("front() = value")
    {
        bytes.front() = 0x18_b;
        THEN("the first value is modified accordingly")
        {
            REQUIRE(bytes[0] == 0x18_b);
        }
    }
    WHEN("value = back()")
    {
        THEN("value is the last value")
        {
            REQUIRE(bytes.back() == bytes[bytes.size() - 1]);
        }
    }
    WHEN("back() = value")
    {
        bytes.back() = 0x18_b;
        THEN("the last value is modified accordingly")
        {
            REQUIRE(bytes[bytes.size() - 1] == 0x18_b);
        }
    }
}
}

SCENARIO(CLASS_UNDER_TEST ": iterating", "[" CLASS_UNDER_TEST "][containers]")
{
GIVEN("A non-empty " CLASS_UNDER_TEST)
{
    small_byte_vector bytes{0x13_b, 0x14_b, 0x15_b};

    WHEN("value = *begin()")
    {
        auto const value = *bytes.begin();
        THEN("value is the first element")
        {
            REQUIRE(value == bytes.front());
        }
    }
    WHEN("value = *cbegin()")
    {
        auto const value = *bytes.cbegin();
        THEN("value is the first element")
        {
            REQUIRE(value == bytes.front());
        }
    }
    WHEN("value = *rbegin()")
    {
        auto const value = *bytes.rbegin();
        THEN("value is the last element")
        {
            REQUIRE(value == bytes.back());
        }
    }
    WHEN("value = *--end()")
    {
        auto const value = *--bytes.end();
        THEN("value is the last element")
        {
            REQUIRE(value == bytes.back());
        }
    }
    WHEN("value = *--cend()")
    {
        auto const value = *--bytes.cend();
        THEN("value is the last element")
        {
            REQUIRE(value == bytes.back());
        }
    }
    WHEN("value = *--rend()")
    {
        auto const value = *--bytes.rend();
        THEN("value is the first element")
        {
            REQUIRE(value == bytes.front());
        }
    }
}
}

SCENARIO(CLASS_UNDER_TEST ": assignment", "[" CLASS_UNDER_TEST "][containers]")
{
GIVEN("a non-empty " CLASS_UNDER_TEST " A (allocator is_always_equal)")
{
    static_assert(small_byte_vector::allocator_traits::is_always_equal::value);

    small_byte_vector A{0x13_b, 0x14_b, 0x15_b};

    AND_GIVEN("another non-empty " CLASS_UNDER_TEST " B")
    {
        small_byte_vector B{0x16_b, 0x17_b, 0x18_b};

        WHEN("A = B")
        {
            A = B;
            THEN("the two compare equal")
            {
                REQUIRE(A == B);
            }
            THEN("the data pointers are different")
            {
                REQUIRE(A.data() != B.data());
            }
        }
        WHEN("A = std::move(B)")
        {
            small_byte_vector old_B{0x16_b, 0x17_b, 0x18_b};
            A = std::move(B);
            THEN("A has the contents of B")
            {
                REQUIRE(A == old_B);
            }
            THEN("B is empty")
            {
                REQUIRE(B.empty());
            }
        }
    }
}
}

SCENARIO(CLASS_UNDER_TEST ": insertion", "[" CLASS_UNDER_TEST "][containers]")
{
GIVEN("a " CLASS_UNDER_TEST " A with at least 2 elements")
{
    small_byte_vector test{0x15_b, 0x16_b};

    WHEN("insert(pos, count, value)")
    {
        AND_WHEN("pos is in the middle")
        {
            test.insert(1, 1, 0x33_b);
            THEN("A.size() increases by count")
            {
                REQUIRE(test.size() == 3);
            }
            THEN("A[pos] == value")
            {
                REQUIRE(test[1] == 0x33_b);
            }
        }
        AND_WHEN("pos is 0")
        {
            test.insert(0, 1, 0x33_b);
            THEN("A[pos] == value")
            {
                REQUIRE(test[0] == 0x33_b);
            }
        }
        AND_WHEN("pos is A.size()-1")
        {
            auto const pos = test.size() - 1;
            test.insert(pos, 1, 0x33_b);
            THEN("A[pos] == value")
            {
                REQUIRE(test[pos] == 0x33_b);
            }
        }
    }
    WHEN("insert(pos_iter, value)")
    {
        AND_WHEN("pos_iter == A.begin()")
        {
            test.insert(test.begin(), 0x34_b);
            THEN("A.size() increases by 1")
            {
                REQUIRE(test.size() == 3);
            }
            THEN("A.front() == value")
            {
                REQUIRE(test.front() == 0x34_b);
            }
        }
        AND_WHEN("pos_iter == A.end()")
        {
            test.insert(test.end(), 0x34_b);
            THEN("A.size() increases by 1")
            {
                REQUIRE(test.size() == 3);
            }
            THEN("A.back() == value")
            {
                REQUIRE(test.back() == 0x34_b);
            }
        }
    }
}
}
*/