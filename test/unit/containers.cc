#include <catch2/catch_all.hpp>
#include <dualis.h>

using namespace dualis;
using namespace dualis::literals;

template <class Allocator, typename Allocator::size_type BufferSize>
auto begin(const dualis::detail::_byte_storage<Allocator, BufferSize>& bytes)
{
    return bytes.data();
}

template <class Allocator, typename Allocator::size_type BufferSize>
auto end(const dualis::detail::_byte_storage<Allocator, BufferSize>& bytes)
{
    return bytes.data() + bytes.size();
}

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

    auto describe() const -> std::string override
    {
        return "\nequals\n" + Catch::rangeToString(m_value);
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
using dont_propagate_not_always_equal_allocator = mock_allocator<std::false_type, std::false_type>;

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

                THEN("_bytes_ == _other_")
                {
                    REQUIRE_THAT(_bytes_, EqualsByteRange(_other_));
                }
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

                THEN("_bytes_ == _other_")
                {
                    REQUIRE_THAT(_bytes_, EqualsByteRange(_other_));
                }
                THEN("the allocator of _other_ has been propagated to _bytes_")
                {
                    REQUIRE(_bytes_.get_allocator().id == _other_.get_allocator().id);

                    AND_THEN("by copying")
                    {
                        REQUIRE(other_stats.copied == 1);
                    }
                    AND_THEN("they compare equal")
                    {
                        REQUIRE(_bytes_.get_allocator() == _other_.get_allocator());
                    }
                }
            }
        }

        WHEN("_bytes_.take_contents(_other);")
        {
            auto _bytes_ = make_embedded_byte_storage<propagate_allocator, EmbeddedSize>(&stats);

            AND_WHEN("_other_.is_allocated()")
            {
                auto _other_ =
                    make_allocated_byte_storage<propagate_allocator, EmbeddedSize>(&other_stats);

                auto const* old_data = _other_.data();
                auto const old_capacity = _other_.capacity();
                auto const old_size = _other_.size();
                _bytes_.take_contents(_other_);

                THEN("_bytes_ is allocated")
                {
                    REQUIRE(_bytes_.is_allocated());
                }
                THEN("_bytes_ has _other_'s data")
                {
                    REQUIRE(_bytes_.data() == old_data);

                    AND_THEN("_bytes_ has _other_'s capacity")
                    {
                        REQUIRE(_bytes_.capacity() == old_capacity);
                    }
                    AND_THEN("_bytes_ has _other_'s size")
                    {
                        REQUIRE(_bytes_.capacity() == old_size);
                    }
                }
                THEN("_other_ is not allocated anymore")
                {
                    REQUIRE_FALSE(_other_.is_allocated());
                }
                THEN("_other_ is empty")
                {
                    REQUIRE(_other_.size() == 0);
                }
            }
            AND_WHEN("!_other_.is_allocated()")
            {
                auto _other_ =
                    make_embedded_byte_storage<propagate_allocator, EmbeddedSize>(&other_stats);
                _other_.assign(TestData.data(), _other_.size());

                auto const old_size = _other_.size();
                _bytes_.take_contents(_other_);

                THEN("_bytes_ is not allocated")
                {
                    REQUIRE_FALSE(_bytes_.is_allocated());
                }
                THEN("_bytes_ has the same size _other_ had")
                {
                    REQUIRE(_bytes_.size() == old_size);

                    AND_THEN("_bytes_ has the same contents _other_ had")
                    {
                        REQUIRE_THAT(_bytes_, EqualsByteRange(TestData, _bytes_.size()));
                    }
                }
                THEN("_other_ is empty")
                {
                    REQUIRE(_other_.size() == 0);
                }
            }
        }

        WHEN("_bytes_.assign(std::move(_other_)); // move-assign")
        {
            AND_WHEN("!allocator_traits::propagate_on_container_move_assignment && "
                     "!allocator_traits::is_always_equal && _bytes_.get_allocator() != "
                     "_other_.get_allocator()")
            {
                auto _bytes_ = make_embedded_byte_storage<dont_propagate_not_always_equal_allocator,
                                                          EmbeddedSize>(&stats);
                auto _other_ = make_embedded_byte_storage<dont_propagate_not_always_equal_allocator,
                                                          EmbeddedSize>(&other_stats, DistinctId);
                _other_.assign(TestData.data(), _other_.size());
                _bytes_.get_allocator().start_tracking();
                _other_.get_allocator().start_tracking();

                auto const old_size = _other_.size();
                _bytes_.assign(std::move(_other_));

                THEN("_other_ is empty")
                {
                    REQUIRE(_other_.size() == 0);
                }
                THEN("_bytes_ has the same size _other_ had")
                {
                    REQUIRE(_bytes_.size() == old_size);

                    AND_THEN("_bytes_ has the same contents _other_ had")
                    {
                        REQUIRE_THAT(_bytes_, EqualsByteRange(TestData, _bytes_.size()));
                    }
                }
                THEN("the allocator of _bytes_ was not touched")
                {
                    REQUIRE_THAT(_bytes_.get_allocator(), NotModified());
                }
                THEN("the allocator of _other_ was not touched")
                {
                    REQUIRE_THAT(_other_.get_allocator(), NotModified());
                }
            }
            AND_WHEN("allocator_traits::propagate_on_container_move_assignment")
            {
                auto _bytes_ =
                    make_embedded_byte_storage<propagate_allocator, EmbeddedSize>(&stats);
                auto _other_ = make_embedded_byte_storage<propagate_allocator, EmbeddedSize>(
                    &other_stats, DistinctId);
                _other_.assign(TestData.data(), _other_.size());
                _bytes_.get_allocator().start_tracking();
                _other_.get_allocator().start_tracking();

                _bytes_.assign(std::move(_other_));

                THEN("the allocator of _bytes_ was replaced by _other_'s")
                {
                    REQUIRE(_bytes_.get_allocator().id == _other_.get_allocator().id);

                    AND_THEN("by moving")
                    {
                        REQUIRE(other_stats.moved == 1);
                    }
                }
            }
            AND_WHEN("!allocator_traits::propagate_on_container_move_assignment")
            {
                auto _bytes_ =
                    make_embedded_byte_storage<dont_propagate_allocator, EmbeddedSize>(&stats);
                auto _other_ = make_embedded_byte_storage<dont_propagate_allocator, EmbeddedSize>(
                    &other_stats);
                _other_.assign(TestData.data(), _other_.size());
                _bytes_.get_allocator().start_tracking();
                _other_.get_allocator().start_tracking();

                _bytes_.assign(std::move(_other_));

                THEN("the allocator of _bytes_ was not touched")
                {
                    REQUIRE_THAT(_bytes_.get_allocator(), NotModified());
                }
                THEN("the allocator of _other_ was not touched")
                {
                    REQUIRE_THAT(_other_.get_allocator(), NotModified());
                }
            }
            AND_WHEN("!_bytes_.is_allocated()")
            {
                auto _bytes_ =
                    make_embedded_byte_storage<dont_propagate_allocator, EmbeddedSize>(&stats);
                auto _other_ = make_embedded_byte_storage<dont_propagate_allocator, EmbeddedSize>(
                    &other_stats);
                _other_.assign(TestData.data(), _other_.size());
                _bytes_.get_allocator().start_tracking();
                _other_.get_allocator().start_tracking();

                _bytes_.assign(std::move(_other_));

                THEN("no deallocation by _bytes_ takes place")
                {
                    REQUIRE(stats.deallocated == 0);
                }
            }
            AND_WHEN("_bytes_.is_allocated()")
            {
                auto _bytes_ =
                    make_allocated_byte_storage<dont_propagate_allocator, EmbeddedSize>(&stats);
                auto _other_ = make_embedded_byte_storage<dont_propagate_allocator, EmbeddedSize>(
                    &other_stats);
                _other_.assign(TestData.data(), _other_.size());
                _bytes_.get_allocator().start_tracking();
                _other_.get_allocator().start_tracking();

                _bytes_.assign(std::move(_other_));

                THEN("the old data of _bytes_ is deallocated")
                {
                    REQUIRE(stats.deallocated == 1);
                }
            }
        }

        WHEN("_bytes_.swap(_other);")
        {
        }
    }
}
