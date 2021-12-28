#pragma once

#include <cstddef>
#include <concepts>
#include <iterator>
#include <type_traits>

namespace dualis {

// clang-format off
template <class T>
concept readable_bytes = requires(const T& t)
{
    { t.data() } -> std::same_as<const std::byte*>;
    { t.size() } -> std::unsigned_integral;
};

template <class T>
concept writable_bytes = requires(T& t)
{
    { t.data() } -> std::same_as<std::byte*>;
    { t.size() } -> std::unsigned_integral;
};

template <class T>
concept insertable_bytes = requires(const T& t)
{
    { t.begin() } -> std::forward_iterator;
    { t.end() } -> std::forward_iterator;
    t.insert(t.begin());
};

template <class T>
concept size_constructible_bytes = requires(std::size_t size, T& t) {
    T{size};
    { t.data() } -> std::same_as<std::byte*>;
};

template<class T>
concept byte_packing = requires(const std::byte* bytes, std::byte* writableBytes,
                                typename T::value_type value)
{
    typename T::value_type;
    { T::unpack(bytes) } -> std::same_as<typename T::value_type>;
    { T::size() } -> std::same_as<std::size_t>;
    T::pack(value, writableBytes);
};
// clang-format on

} // namespace dualis
