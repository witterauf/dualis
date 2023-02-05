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

namespace detail {

inline auto check_base64(std::string_view base64) -> std::optional<size_t>
{
    if (base64.length() == 1)
    {
        // Only 0, 2, and 3 characters are valid sequences.
        return std::nullopt;
    }
    size_t i{0};
    for (; i < base64.length() && base64[i] != '='; ++i)
    {
        const char c = base64[i];
        if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
              c == '+' || c == '/'))
        {
            return std::nullopt;
        }
    }
    auto const length = i;
    size_t padding{0};
    for (; i < base64.length() && base64[i] == '='; ++i, ++padding)
    {
    }
    if (i != base64.length() && padding > 2)
    {
        return std::nullopt;
    }
    return length;
}

} // namespace detail

template <size_constructible_bytes Bytes>
auto from_base64(std::string_view base64) -> std::optional<Bytes>
{
    static constexpr uint8_t Values[] = {
        // clang-format off
        // '+' = 0x2b
        0x3e, 0x00, 0x00, 0x00, 0x3f,
        // '0' = 0x30
        0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b,
        0x3c, 0x3d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00,
        // starting at 'A' = 0x41
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        // 'a' == 0x61
        0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21,
        0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29,
        0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31,
        0x32, 0x33,
        // clang-format on
    };

    auto const maybeLength = detail::check_base64(base64);
    if (!maybeLength)
    {
        return std::nullopt;
    }
    auto const bits = *maybeLength * 6;
    auto const size = bits / 8;
    Bytes bytes(size);
    size_t i = 0, j = 0;

    auto transform = [&bytes](size_t j, uint8_t value0, uint8_t value1, uint8_t value2,
                              uint8_t value3) {
        bytes[j + 0] = static_cast<std::byte>((value0 << 2) | (value1 >> 4));
        bytes[j + 1] = static_cast<std::byte>((value1 << 4) | (value2 >> 2));
        bytes[j + 2] = static_cast<std::byte>((value2 << 6) | value3);
    };

    for (; i + 4 <= base64.length(); i += 4, j += 3)
    {
        transform(j, Values[static_cast<size_t>(base64[i + 0]) - 0x2b],
                  Values[static_cast<size_t>(base64[i + 1]) - 0x2b],
                  Values[static_cast<size_t>(base64[i + 2]) - 0x2b],
                  Values[static_cast<size_t>(base64[i + 3]) - 0x2b]);
    }
    if (i < base64.length())
    {
        auto const diff = base64.length() - i;
        auto const value0 = Values[static_cast<size_t>(base64[i + 0]) - 0x2b];
        auto const value1 = Values[static_cast<size_t>(base64[i + 1]) - 0x2b];
        bytes[j + 0] = static_cast<std::byte>((value0 << 2) | (value1 >> 4));
        if (diff == 1)
        {
            auto const value2 = Values[static_cast<size_t>(base64[i + 2]) - 0x2b];
            bytes[j + 1] = static_cast<std::byte>((value1 << 4) | (value2 >> 2));
        }
    }
    return bytes;
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