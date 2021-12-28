#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace dualis {

/// Swaps the byte order of the given integer.
template <std::unsigned_integral T> auto byte_swap(T value) -> T;

template <std::size_t Width> class unsigned_int;

template <> struct unsigned_int<2>
{
    using type = uint16_t;
};

template <> struct unsigned_int<4>
{
    using type = uint32_t;
};

template <> inline auto byte_swap<uint8_t>(uint8_t value) -> uint8_t
{
    return value;
}

template <> inline auto byte_swap<uint16_t>(uint16_t value) -> uint16_t
{
#ifdef _MSC_VER
    return _byteswap_ushort(value);
#elif __GNUG__
    return __builtin_bswap16(value);
#else
    return (value << 8) | (value >> 8);
#endif
}

template <> inline auto byte_swap<uint32_t>(uint32_t value) -> uint32_t
{
#ifdef _MSC_VER
    return _byteswap_ulong(value);
#elif __GNUG__
    return __builtin_bswap32(value);
#else
    return (value << 24) | ((value << 8) & 0xff0000) | ((value >> 8) & 0xff00) | (value >> 24);
#endif
}

template <> inline auto byte_swap<uint64_t>(uint64_t value) -> uint64_t
{
#ifdef _MSC_VER
    return _byteswap_uint64(value);
#elif __GNUG__
    return __builtin_bswap64(value);
#else
    return (value << 56) | ((value << 40) & (0xffULL << 48)) | ((value << 24) & (0xffULL << 40)) |
           ((value << 8) & (0xffULL << 32)) | ((value >> 8) & (0xffULL << 24)) |
           ((value >> 24) & (0xffULL << 16) | ((value >> 40) & (0xffULL << 8))) | (value >> 56);
#endif
}

// Use compiler-defined intrinsics if available.
inline auto copy_bytes(std::byte* dest, const std::byte* src, std::size_t size)
{
    std::memcpy(dest, src, size);
}

namespace detail {

static constexpr char HexDigits[] = {'0', '1', '2', '3', '4', '5', '6', '7',
                                     '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

}

inline auto to_hex_string(const std::byte value) -> std::string
{
    return std::string{detail::HexDigits[std::to_integer<uint8_t>(value) >> 4],
                       detail::HexDigits[std::to_integer<uint8_t>(value) & 0xf]};
}

namespace literals {

consteval auto operator"" _b(unsigned long long int value) -> std::byte
{
    if (value >= 0x100)
    {
        ;
    }
    return std::byte{static_cast<uint8_t>(value)};
}
} // namespace literals
} // namespace dualis