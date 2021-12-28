#pragma once

#include "utilities.h"
#include <concepts>
#include <cstdint>
#include <type_traits>

namespace dualis {

namespace detail {

template <std::integral T> class _little_endian_ptrcast final
{
public:
    using value_type = T;

    static auto unpack(const std::byte* bytes) -> T
    {
        return *reinterpret_cast<const T*>(bytes);
    }

    static void pack(T value, std::byte* bytes)
    {
        *reinterpret_cast<T*>(bytes) = value;
    }

    static constexpr auto size()
    {
        return sizeof(value_type);
    }
};

template <std::integral T> class _big_endian_ptrcast final
{
public:
    using value_type = T;

    static auto unpack(const std::byte* bytes) -> T
    {
        return ::dualis::byte_swap(
            static_cast<typename std::make_unsigned<T>::type>(*reinterpret_cast<const T*>(bytes)));
    }

    static void pack(T value, std::byte* bytes)
    {
        *reinterpret_cast<T*>(bytes) =
            ::dualis::byte_swap(static_cast<typename std::make_unsigned<T>::type>(value));
    }

    static constexpr auto size()
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

// default aliases

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

template <std::default_initializable T> struct raw
{
    using value_type = T;

    static auto unpack(const std::byte* bytes) -> T
    {
        T result{};
        copy_bytes(reinterpret_cast<std::byte*>(&result), bytes, size());
        return result;
    }

    static void pack(const T& value, std::byte* bytes)
    {
        copy_bytes(bytes, reinterpret_cast<const std::byte*>(&value), size());
    }

    static constexpr auto size()
    {
        return sizeof(value_type);
    }
};

template <byte_packing Packing, readable_bytes Bytes>
auto unpack(const Bytes& bytes, std::size_t offset) -> typename Packing::value_type
{
    return Packing::unpack(bytes.data() + offset);
}

template <byte_packing Packing, class U, writable_bytes Bytes>
void pack(Bytes& bytes, std::size_t offset, const U& value)
{
    Packing::pack(typename Packing::value_type(value), bytes.data() + offset);
}

namespace detail {

template <byte_packing Packing>
auto _unpack_from(const std::byte* bytes, Packing packing)
    -> std::tuple<typename Packing::value_type>
{
    return std::make_tuple(Packing::unpack(bytes));
}

template <byte_packing Packing, byte_packing... Packings>
auto _unpack_from(const std::byte* bytes, Packing packing, Packings... packings)
{
    auto const value = Packing::unpack(bytes);
    auto const remaining = _unpack_from(bytes + Packing::size(), packings...);
    return std::tuple_cat(std::make_tuple(value), remaining);
}

template <byte_packing Packing, byte_packing... Packings>
void _pack_into(std::byte* bytes, typename Packing::value_type value,
                typename Packings::value_type... values)
{
    Packing::pack(value, bytes);
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

template <byte_packing... Packings> struct sequence
{
    using value_type = std::tuple<typename Packings::value_type...>;

    static auto unpack(const std::byte* bytes) -> value_type
    {
        return detail::_unpack_from(bytes, Packings{}...);
    }

    static void pack(const value_type& value, std::byte* bytes)
    {
        detail::_pack_into<0, Packings...>(bytes, value);
    }

    static void pack(std::byte* bytes, typename Packings::value_type... values)
    {
        detail::_pack_into<Packings...>(bytes, values...);
    }

    static constexpr auto size()
    {
        return (Packings::size() + ...);
    }
};

template <byte_packing... Packings, readable_bytes Bytes>
auto unpack_sequence(const Bytes& bytes, std::size_t offset)
{
    return unpack<sequence<Packings...>>(bytes, offset);
}

template <byte_packing... Packings, writable_bytes Bytes>
void pack_sequence(Bytes& bytes, std::size_t offset, typename Packings::value_type... values)
{
    sequence<Packings...>::pack(bytes.data() + offset, values...);
}

template <byte_packing Packing, readable_bytes Bytes, class OutputIt>
auto unpack_n(OutputIt first, std::size_t count, const Bytes& bytes, std::size_t offset = 0)
    -> OutputIt
{
    for (decltype(count) i = 0; i < count; ++i)
    {
        *first++ = unpack<Packing>(bytes, offset);
        offset += Packing::size();
    }
    return first;
}

} // namespace dualis
