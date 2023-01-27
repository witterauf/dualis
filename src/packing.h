#pragma once

#include "utilities.h"
#include <concepts>
#include <cstdint>
#include <ranges>
#include <type_traits>

namespace dualis {

///////////////////////////////////////////////////////////////////////////////////////////////////
// Packings
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace detail {

// Implements packing of an integral type T into bytes with little endian byte order by casting the
// given std::byte pointer to a pointer to T. This is fast, but assumes the CPU supports unaligned
// access (since the byte pointer may point to an unaligned address).
template <std::integral T> class _little_endian_ptrcast final
{
public:
    using value_type = T;

    [[nodiscard]] static auto unpack(const std::byte* bytes) -> T
    {
        return *reinterpret_cast<const T*>(bytes);
    }

    static void pack(std::byte* bytes, T value)
    {
        *reinterpret_cast<T*>(bytes) = value;
    }

    [[nodiscard]] static constexpr auto size()
    {
        return sizeof(value_type);
    }
};

// Implements packing of an integral type T into bytes with big endian byte order by casting the
// given std::byte pointer to a pointer to T. This is fast, but assumes the CPU supports unaligned
// access (since the byte pointer may point to an unaligned address).
template <std::integral T> class _big_endian_ptrcast final
{
public:
    using value_type = T;

    [[nodiscard]] static auto unpack(const std::byte* bytes) -> T
    {
        return ::dualis::byte_swap(
            static_cast<typename std::make_unsigned<T>::type>(*reinterpret_cast<const T*>(bytes)));
    }

    static void pack(std::byte* bytes, T value)
    {
        *reinterpret_cast<T*>(bytes) =
            ::dualis::byte_swap(static_cast<typename std::make_unsigned<T>::type>(value));
    }

    [[nodiscard]] static constexpr auto size()
    {
        return sizeof(value_type);
    }
};

} // namespace detail

#ifdef _DUALIS_UNALIGNED_MEM_ACCESS
template <std::integral T> using little_endian = detail::_little_endian_ptrcast<T>;
template <std::integral T> using big_endian = detail::_big_endian_ptrcast<T>;
#else
#error "Only architectures that allow unaligned memory access supported so far"
#endif

using uint16_le = little_endian<uint16_t>;
using uint32_le = little_endian<uint32_t>;
using uint64_le = little_endian<uint64_t>;
using int16_le = little_endian<int16_t>;
using int32_le = little_endian<int32_t>;
using int64_le = little_endian<int64_t>;
using uint16_be = big_endian<uint16_t>;
using uint32_be = big_endian<uint32_t>;
using uint64_be = big_endian<uint64_t>;
using int16_be = big_endian<int16_t>;
using int32_be = big_endian<int32_t>;
using int64_be = big_endian<int64_t>;

static_assert(byte_packing<uint16_le>);
static_assert(byte_packing<int16_be>);

// Implements packing of any default-initializable type T into bytes and from bytes using the memory
// layout given by the compiler. This might not match across different compilers (e.g. alignment,
// struct packing) and architectures (e.g. big vs. little endian), so use with care.
template <std::default_initializable T> struct raw
{
    using value_type = T;

    [[nodiscard]] static auto unpack(const std::byte* bytes) -> T
    {
        T result{};
        copy_bytes(reinterpret_cast<std::byte*>(&result), bytes, size());
        return result;
    }

    static void pack(std::byte* bytes, const T& value)
    {
        copy_bytes(bytes, reinterpret_cast<const std::byte*>(&value), size());
    }

    [[nodiscard]] static constexpr auto size()
    {
        return sizeof(value_type);
    }
};

namespace detail {

template <byte_packing Packing>
[[nodiscard]] auto _unpack_from(const std::byte* bytes, Packing)
    -> std::tuple<typename Packing::value_type>
{
    return std::make_tuple(Packing::unpack(bytes));
}

template <byte_packing Packing, byte_packing... Packings>
[[nodiscard]] auto _unpack_from(const std::byte* bytes, Packing, Packings... packings)
{
    auto const value = Packing::unpack(bytes);
    auto const remaining = _unpack_from(bytes + Packing::size(), packings...);
    return std::tuple_cat(std::make_tuple(value), remaining);
}

template <byte_packing Packing, byte_packing... Packings>
void _pack_into(std::byte* bytes, typename Packing::value_type value,
                typename Packings::value_type... values)
{
    Packing::pack(bytes, value);
    if constexpr (sizeof...(Packings) > 0)
    {
        _pack_into<Packings...>(bytes + Packing::size(), values...);
    }
}

template <std::size_t Index, byte_packing... Packings>
void _pack_into(std::byte* bytes, const std::tuple<typename Packings::value_type...>& values)
{
    if constexpr (Index < sizeof...(Packings))
    {
        using Packing = std::tuple_element_t<Index, std::tuple<Packings...>>;
        Packing::pack(std::get<Index>(values), bytes);
        _pack_into<Index + 1, Packings...>(bytes + Packing::size(), values);
    }
}

} // namespace detail

// Packs or unpacks multiple values of differing types in sequence. Each type must be specified as
// another packing.
template <byte_packing... Packings> struct tuple_packing
{
    static_assert(sizeof...(Packings) > 0);
    using value_type = std::tuple<typename Packings::value_type...>;

    [[nodiscard]] static auto unpack(const std::byte* bytes) -> value_type
    {
        return detail::_unpack_from(bytes, Packings{}...);
    }

    static void pack(std::byte* bytes, const value_type& value)
    {
        detail::_pack_into<0, Packings...>(bytes, value);
    }

    // Convenience method for pack_tuple that circumvents the construction of an std::tuple.
    static void pack(std::byte* bytes, const typename Packings::value_type&... values)
    {
        detail::_pack_into<Packings...>(bytes, values...);
    }

    [[nodiscard]] static constexpr auto size()
    {
        return (Packings::size() + ...);
    }
};

// Make sure tuple_packing is a packing using example arguments.
static_assert(byte_packing<tuple_packing<uint16_le>>);
static_assert(byte_packing<tuple_packing<uint16_le, uint16_le>>);

///////////////////////////////////////////////////////////////////////////////////////////////////
// Packing of singular type
///////////////////////////////////////////////////////////////////////////////////////////////////

template <byte_packing Packing, byte_range Bytes>
[[nodiscard]] auto unpack(const Bytes& bytes, std::size_t offset) -> typename Packing::value_type
{
    return Packing::unpack(std::ranges::cdata(bytes) + offset);
}

template <byte_packing Packing, class U, writable_byte_range Bytes>
void pack(Bytes& bytes, std::size_t offset, const U& value)
{
    Packing::pack(std::ranges::data(bytes) + offset, typename Packing::value_type(value));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Compound packing (multiple types, multiple values of same type)
///////////////////////////////////////////////////////////////////////////////////////////////////

template <byte_packing... Packings, byte_range Bytes>
[[nodiscard]] auto unpack_tuple(const Bytes& bytes, std::size_t offset)
{
    return unpack<tuple_packing<Packings...>>(bytes, offset);
}

template <byte_packing... Packings, writable_byte_range Bytes>
void pack_tuple(Bytes& bytes, std::size_t offset, const typename Packings::value_type&... values)
{
    tuple_packing<Packings...>::pack(std::ranges::data(bytes) + offset, values...);
}

// Unpacks n values into the given output iterator.
template <byte_packing Packing, byte_range Bytes, class Iterator>
requires std::output_iterator<Iterator, typename Packing::value_type>
auto unpack_range(const Bytes& bytes, std::size_t offset, Iterator first, std::size_t count)
    -> Iterator
{
    for (decltype(count) i = 0; i < count; ++i)
    {
        *first++ = unpack<Packing>(bytes, offset);
        offset += Packing::size();
    }
    return first;
}

template <byte_packing Packing, writable_byte_range Bytes, std::input_iterator Iterator>
void pack_range(Bytes& bytes, std::size_t offset, Iterator first, Iterator last)
{
    while (first != last)
    {
        pack<Packing>(bytes, offset, *first++);
        offset += Packing::size();
    }
}

template <byte_packing Packing, writable_byte_range Bytes, std::ranges::range Range>
void pack_range(Bytes& bytes, std::size_t offset, const Range& range)
{
    for (auto const& value : range)
    {
        pack<Packing>(bytes, offset, value);
        offset += Packing::size();
    }
}

} // namespace dualis
