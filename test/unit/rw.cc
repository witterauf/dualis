#include <catch2/catch_all.hpp>
#include <dualis.h>

using namespace dualis;

SCENARIO("read_integer", "[reading]")
{
    using namespace dualis::literals;

    GIVEN("a byte_vector with enough elements")
    {
        byte_vector bv{0x80_b, 0x11_b, 0x12_b, 0x83_b, 0x10_b, 0x11_b, 0x12_b, 0x83_b};
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
                AND_WHEN("T = int32_t")
                {
                    using T = int32_t;
                    AND_WHEN("offset is aligned")
                    {
                        auto const result = read_integer<T, WordOrder>(bv, 0);
                        THEN("result is correct")
                        {
                            REQUIRE(result == (std::to_integer<T>(bv[0]) << 24) +
                                                  ((std::to_integer<T>(bv[1]) & 0xff) << 16) +
                                                  ((std::to_integer<T>(bv[2]) & 0xff) << 8) +
                                                  ((std::to_integer<T>(bv[3]) & 0xff) << 0));
                            REQUIRE(result < 0);
                        }
                    }
                }
            }
        }
    }
}

SCENARIO("write_integer", "[reading]")
{
    using namespace dualis::literals;

    GIVEN("a byte_vector with enough space")
    {
        byte_vector bv{0x80_b, 0x11_b, 0x12_b, 0x83_b, 0x10_b, 0x11_b, 0x12_b, 0x83_b};
        WHEN("write_integer<T, WordOrder>(bv, value, offset)")
        {
            AND_WHEN("WordOrder = little_endian")
            {
                using WordOrder = little_endian;
                AND_WHEN("T = int32_t")
                {
                    using T = int32_t;
                    AND_WHEN("offset is aligned")
                    {
                        write_integer<T, WordOrder>(bv, -123, 0);
                        THEN("value's byte are written in the correct order")
                        {
                            REQUIRE(-123 == (std::to_integer<T>(bv[0]) << 0) +
                                                (std::to_integer<T>(bv[1]) << 8) +
                                                (std::to_integer<T>(bv[2]) << 16) +
                                                (std::to_integer<T>(bv[3]) << 24));
                        }
                    }
                }
            }
        }
    }
}
