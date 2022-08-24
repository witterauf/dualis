#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace dualis {

namespace detail {

// Provide convenient, parameterized access to integers of a given width.
template <std::size_t Width> class _unsigned_int;
template <std::size_t Width> class _signed_int;

template <> struct _unsigned_int<1>
{
    using type = uint8_t;
};

template <> struct _signed_int<1>
{
    using type = int8_t;
};

template <> struct _unsigned_int<2>
{
    using type = uint16_t;
};

template <> struct _signed_int<2>
{
    using type = int16_t;
};

template <> struct _unsigned_int<4>
{
    using type = uint32_t;
};

template <> struct _signed_int<4>
{
    using type = int32_t;
};

template <> struct _unsigned_int<8>
{
    using type = uint64_t;
};

template <> struct _signed_int<8>
{
    using type = int64_t;
};

} // namespace detail

template <std::size_t Width> using unsigned_int = typename detail::_unsigned_int<Width>::type;
template <std::size_t Width> using signed_int = typename detail::_signed_int<Width>::type;

// Swaps the byte order of the given integer.
template <std::unsigned_integral T> [[nodiscard]] auto byte_swap(T value) -> T;

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
inline void copy_bytes(std::byte* dest, const std::byte* src, std::size_t size)
{
#ifdef __GNUG__
    __builtin_memcpy(dest, src, size);
#else
    std::memcpy(dest, src, size);
#endif
}

inline void move_bytes(std::byte* dest, const std::byte* src, std::size_t size)
{
#ifdef __GNUG__
    __builtin_memmove(dest, src, size);
#else
    std::memmove(dest, src, size);
#endif
}

inline void set_bytes(std::byte* dest, std::byte value, std::size_t size)
{
#ifdef __GNUG__
    __builtin_memset(dest, std::to_integer<int>(value), size);
#else
    std::memset(dest, std::to_integer<int>(value), size);
#endif
}

inline int compare_bytes(const std::byte* a, const std::byte* b, std::size_t size)
{
#ifdef __GNUG__
    return __builtin_memcmp(a, b, size);
#else
    return std::memcmp(a, b, size);
#endif
}

// From https://github.com/microsoft/GSL/blob/main/include/gsl/util
template <class T, class U> [[nodiscard]] constexpr T narrow_cast(U&& u) noexcept
{
    return static_cast<T>(std::forward<U>(u));
}

namespace detail {

static constexpr char HexDigits[] = {'0', '1', '2', '3', '4', '5', '6', '7',
                                     '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

static constexpr char BinaryDigits[] = {'0', '1'};

} // namespace detail

inline auto to_hex_string(const std::byte value) -> std::string
{
    return std::string{detail::HexDigits[std::to_integer<uint8_t>(value) >> 4],
                       detail::HexDigits[std::to_integer<uint8_t>(value) & 0xf]};
}

inline auto to_hex_string(long long value) -> std::string
{
    if (value == 0)
    {
        return "0";
    }

    std::string hex;
    while (value != 0)
    {
        hex += detail::HexDigits[value & 0xf];
        value >>= 4;
    }
    return std::string(hex.crbegin(), hex.crend());
}

inline auto to_binary_string(long long value) -> std::string
{
    if (value == 0)
    {
        return "0";
    }

    std::string result;
    while (value != 0)
    {
        result += detail::BinaryDigits[value & 1];
        value >>= 1;
    }
    return std::string(result.crbegin(), result.crend());
}

template <byte_range Bytes> auto to_base64(const Bytes& bytes) -> std::string
{
    static constexpr char Digits[65] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    auto const length = std::ranges::size(bytes);
    auto const* data = reinterpret_cast<const uint8_t*>(std::ranges::cdata(bytes));
    size_t offset = 0;
    std::string base64;
    for (; offset + 3 <= length; offset += 3)
    {
        uint8_t separated;
        separated = data[offset] >> 2;
        base64 += Digits[separated];
        separated = ((data[offset] & 0x03) << 4) | (data[offset + 1] >> 4);
        base64 += Digits[separated];
        separated = ((data[offset + 1] & 0x0f) << 2) | (data[offset + 2] >> 6);
        base64 += Digits[separated];
        separated = data[offset + 2] & 0x3f;
        base64 += Digits[separated];
    }
    if (offset < length)
    {
        if (length - offset == 2)
        {
            uint8_t separated;
            separated = data[offset] >> 2;
            base64 += Digits[separated];
            separated = ((data[offset] & 0x03) << 4) | (data[offset + 1] >> 4);
            base64 += Digits[separated];
            separated = (data[offset + 1] & 0x0f) << 2;
            base64 += Digits[separated];
            base64 += '=';
        }
        if (length - offset == 1)
        {
            uint8_t separated;
            separated = data[offset] >> 2;
            base64 += Digits[separated];
            separated = (data[offset] & 0x03) << 4;
            base64 += Digits[separated];
            base64 += '=';
            base64 += '=';
        }
    }
    return base64;
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