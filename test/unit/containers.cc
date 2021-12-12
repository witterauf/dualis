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

#define CLASS_UNDER_TEST "small_byte_vector"

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
