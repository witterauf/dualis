#include <catch2/catch_all.hpp>
#include <dualis.h>

using namespace dualis;

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

SCENARIO("small_byte_vector::small_byte_vector", "[small_byte_vector][containers]")
{
    WHEN("default-constructing a byte_vector")
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
}