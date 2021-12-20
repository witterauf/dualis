#include <catch2/catch_all.hpp>
#include <dualis.h>

using namespace dualis;
using namespace dualis::literals;

SCENARIO("byte_vector::byte_vector", "[byte_vector]")
{
    WHEN("default-constructing a byte_vector")
    {
        byte_vector bytes;
        THEN("its size is 0")
        {
            REQUIRE(bytes.size() == 0);
        }
        THEN("it is considered empty")
        {
            REQUIRE(bytes.empty());
        }
        THEN("its begin and end iterators are equal")
        {
            REQUIRE(bytes.begin() == bytes.end());
        }
    }

    GIVEN("a size N")
    {
        constexpr byte_vector::size_type N = 32;
        WHEN("constructing a byte_vector with size N")
        {
            byte_vector bytes(N);
            THEN("its size is N")
            {
                REQUIRE(bytes.size() == N);
            }
            THEN("its begin and end iterators are N apart")
            {
                REQUIRE(std::distance(bytes.begin(), bytes.end()) == N);
            }

            AND_WHEN("N > 0")
            {
                THEN("it is considered non-empty")
                {
                    REQUIRE_FALSE(bytes.empty());
                }
            }
        }
    }
}

SCENARIO("byte_vector::operator+=", "[byte_vector][containers]")
{
    using namespace dualis::literals;

    GIVEN("a byte_vector bvA with size N")
    {
        byte_vector bvA{0x12_b, 0x13_b, 0x14_b};
        auto const N = bvA.size();
        AND_GIVEN("a byte_vector bvB with size M")
        {
            byte_vector bvB{0x15_b, 0x16_b, 0x17_b};
            auto const M = bvB.size();
            WHEN("bvA += bvB")
            {
                auto const bvOldA = bvA;
                bvA += bvB;
                THEN("the size of bvA becomes N+M")
                {
                    REQUIRE(bvA.size() == N + M);
                }
                THEN("the first N elements stay the same")
                {
                    REQUIRE(std::equal(bvOldA.cbegin(), bvOldA.cend(), bvA.cbegin()));

                    AND_THEN("the next M elements are from bvB")
                    {
                        REQUIRE(std::equal(bvB.cbegin(), bvB.cend(), bvA.cbegin() + N));
                    }
                }
            }
        }
    }
}

SCENARIO("byte_vector::slice", "[byte_vector][containers]")
{
    using namespace dualis::literals;

    GIVEN("a byte_vector bv with N > 0 elements")
    {
        byte_vector bv{0x10_b, 0x11_b, 0x12_b, 0x13_b, 0x14_b};
        WHEN("result = bv.slice(first, last)")
        {
            auto const first = 1;
            auto const last = 3;
            auto const result = bv.slice(first, last);

            THEN("result has length last-first")
            {
                REQUIRE(result.size() == last - first);
            }
            THEN("result has the corresponding elements")
            {
                REQUIRE(std::equal(result.cbegin(), result.cend(), bv.cbegin() + first));
            }
        }
    }
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
    EqualsByteRangeMatcher(const ByteRange& value)
        : m_value{value}
    {
    }

    template <typename OtherRange> bool match(OtherRange const& other) const
    {
        if (m_value.size() != other.size())
        {
            return false;
        }
        auto thisBegin = m_value.begin();
        auto otherBegin = other.begin();
        for (; thisBegin != m_value.end(); ++thisBegin, ++otherBegin)
        {
            if (*thisBegin != *otherBegin)
            {
                return false;
            }
        }
        return true;
    }

    std::string describe() const override
    {
        return "Equals";
    }

private:
    const ByteRange& m_value;
};

template <class ByteRange>
auto EqualsByteRange(const ByteRange& bytes) -> EqualsByteRangeMatcher<ByteRange>
{
    return EqualsByteRangeMatcher<ByteRange>{bytes};
}

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
    {
        stats->copied += 1;
    }
    constexpr mock_allocator(mock_allocator&& other) noexcept
        : allocator{std::move(other.allocator)}
        , stats{other.stats}
    {
        stats->moved += 1;
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

/*

SCENARIO("small_byte_vector::small_byte_vector", "[small_byte_vector][containers]")
{
WHEN("this()")
{
    small_byte_vector bytes;
    THEN("its size is 0")
    {
        REQUIRE(bytes.size() == 0);
    }
    THEN("it is considered empty")
    {
        REQUIRE(bytes.empty());
    }
    THEN("its begin and end iterators are equal")
    {
        REQUIRE(bytes.begin() == bytes.end());
        auto const& const_bytes = bytes;
        REQUIRE(const_bytes.begin() == const_bytes.end());
        REQUIRE(bytes.cbegin() == bytes.cend());
    }
}

GIVEN("a size N")
{
    static constexpr std::size_t N = 13;
    WHEN("this(N)")
    {
        small_byte_vector bytes(N);
        requireSizeN(bytes, N);
    }
    WHEN("this(N, A)")
    {
        constexpr static std::byte A = 123_b;
        small_byte_vector bytes(N, A);

        requireSizeN(bytes, N);
        THEN("all its element are A")
        {
            REQUIRE_THAT(bytes, EachElementIs(A));
        }
    }
}

GIVEN("an initializer_list init")
{
    static constexpr auto TestBytes = {0x13_b, 0x14_b, 0x15_b};

    WHEN("this(init)")
    {
        small_byte_vector bytes{TestBytes};

        THEN("this has the same elements as init")
        {
            REQUIRE_THAT(bytes, EqualsByteRange(TestBytes));
        }
    }
}

GIVEN("another small_byte_vector other")
{
    static constexpr auto TestBytes = {0x13_b, 0x14_b, 0x15_b};
    small_byte_vector other{TestBytes};

    WHEN("this(other)")
    {
        small_byte_vector bytes{other};

        THEN("they contain the same elements")
        {
            REQUIRE_THAT(bytes, EqualsByteRange(other));
        }
        THEN("their data pointers are different")
        {
            REQUIRE(bytes.data() != other.data());
        }
    }

    WHEN("this(std::move(other))")
    {
        small_byte_vector bytes{std::move(other)};

        THEN("this contains the same elements as other did")
        {
            REQUIRE_THAT(bytes, EqualsByteRange(TestBytes));
        }
        THEN("other is considered empty")
        {
            REQUIRE(other.empty());
        }
    }
}
}

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