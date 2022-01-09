#pragma once

#include <concepts>
#include <cstddef>
#include <iterator>
#include <ranges>
#include <type_traits>

namespace dualis {

// clang-format off
template <class R>
concept byte_range = std::ranges::contiguous_range<R> &&
                     std::same_as<std::remove_cv_t<std::ranges::range_value_t<R>>, std::byte>;

template <class R>
concept writable_byte_range = byte_range<R> && requires(R& r)
{
    { std::ranges::data(r) } -> std::same_as<std::byte*>;
};

template <class T>
concept size_constructible_bytes = requires(std::size_t size, T& t) {
    T{size};
    { t.data() } -> std::same_as<std::byte*>;
};

// A byte_packing describes how a given datum of a type is packed into or unpacked from an array of
// bytes, as well as how many bytes it takes.
template<class T>
concept byte_packing = requires(const std::byte* bytes,
                                std::byte* writable_bytes,
                                const typename T::value_type& value)
{
    typename T::value_type;
    { T::unpack(bytes) } -> std::same_as<typename T::value_type>;
    { T::size() } -> std::same_as<std::size_t>;
    T::pack(writable_bytes, value);
};
// clang-format on

} // namespace dualis
