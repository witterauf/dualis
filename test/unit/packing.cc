#include <catch2/catch_all.hpp>
#include <dualis.h>
#include <vector>

using namespace dualis;
using namespace dualis::literals;

SCENARIO("Unpacking bytes", "[unpacking]")
{
    GIVEN("a sequence of bytes")
    {
        std::vector<std::byte> bytes{0x80_b, 0x10_b, 0x11_b, 0x12_b};

        WHEN("unpacking two values")
        {
            auto const [value1, value2] = unpack_sequence<uint16_le, uint16_le>(bytes, 0);

            THEN("the first value is correct")
            {
                REQUIRE(value1 == 0x1080);
            }
            THEN("the second value is correct")
            {
                REQUIRE(value2 == 0x1211);
            }
        }
    }
}

SCENARIO("Packing into bytes", "[packing]")
{
    GIVEN("a byte container")
    {
        std::vector<std::byte> bytes(16, 0x00_b);

        WHEN("packing two values")
        {
            pack_sequence<uint16_le, uint16_le>(bytes, 0, 1111, 2222);

            REQUIRE(std::to_integer<int>(bytes[0]) == (1111 & 0xff));
            REQUIRE(std::to_integer<int>(bytes[1]) == (1111 >> 8));
        }
    }
}