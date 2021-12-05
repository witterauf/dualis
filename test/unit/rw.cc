#include <catch2/catch_all.hpp>
#include <dualis.h>

using namespace dualis;

SCENARIO("read_integer", "[reading]")
{
    using namespace dualis::literals;

    GIVEN("a byte_vector with enough elements")
    {
        byte_vector bv{0x10_b, 0x11_b, 0x12_b, 0x13_b, 0x10_b, 0x11_b, 0x12_b, 0x13_b};
        WHEN("result = read_integer<T, WordOrder>(bv, offset)")
        {
            AND_WHEN("WordOrder = little_endian")
            {
                AND_WHEN("T = uint16_t")
                {
                    AND_WHEN("offset is aligned")
                    {
                        auto const result = read_integer<uint16_t, little_endian>(bv, 0);
                        THEN("result is correct")
                        {
                            REQUIRE(result == std::to_integer<uint16_t>(bv[0]) +
                                                  (std::to_integer<uint16_t>(bv[1]) << 8));
                        }
                    }
                    AND_WHEN("offset is unaligned")
                    {
                        auto const result = read_integer<uint16_t, little_endian>(bv, 1);
                        THEN("result is correct")
                        {
                            REQUIRE(result == std::to_integer<uint16_t>(bv[1]) +
                                                  (std::to_integer<uint16_t>(bv[2]) << 8));
                        }
                    }
                }
            }
            AND_WHEN("WordOrder = big_endian")
            {
                using WordOrder = big_endian;
                AND_WHEN("T = uint32_t")
                {
                    using T = uint32_t;
                    AND_WHEN("offset is aligned")
                    {
                        auto const result = read_integer<T, WordOrder>(bv, 0);
                        THEN("result is correct")
                        {
                            REQUIRE(result == (std::to_integer<T>(bv[0]) << 24) +
                                                  (std::to_integer<T>(bv[1]) << 16) +
                                                  (std::to_integer<T>(bv[2]) << 8) +
                                                  (std::to_integer<T>(bv[3]) << 0));
                        }
                    }
                }
            }
        }
    }
}