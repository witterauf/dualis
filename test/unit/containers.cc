#include <catch2/catch_all.hpp>
#include <dualis.h>

using namespace dualis;

SCENARIO("constructing byte_vector", "[byte_vector]")
{
    GIVEN("nothing")
    {
        byte_vector bytes;

        WHEN("default-constructing a byte_vector")
        {
            THEN("its size is 0")
            {
                REQUIRE(bytes.size() == 0);
            }
            THEN("it is considered empty")
            {
                REQUIRE(bytes.empty());
            }
        }
    }

    GIVEN("a size N")
    {
        constexpr byte_vector::size_type N = 32;
        byte_vector bytes(N);

        WHEN("constructing a byte_vector with this size")
        {
            THEN("its size is N")
            {
                REQUIRE(bytes.size() == N);
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
