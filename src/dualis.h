#pragma once

#include <concepts>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <span>
#include <vector>

#define _DUALIS_UNALIGNED_MEM_ACCESS

namespace dualis {

// clang-format off
template <class T>
concept readable_bytes = requires(const T& t)
{
    { t.data() } -> std::same_as<const std::byte*>;
    { t.size() } -> std::unsigned_integral;
};

template <class T>
concept insertable_bytes = requires(const T& t)
{
    { t.begin() } -> std::forward_iterator;
    { t.end() } -> std::forward_iterator;
    t.insert(t.begin());
};

template<class T, class U>
concept word_order = requires(const std::byte* bytes)
{
    { T::template read<U>(bytes) } -> std::same_as<U>;
};
// clang-format on

using writable_byte_span = std::span<std::byte>;
using byte_span = std::span<const std::byte>;

template <class Allocator = std::allocator<std::byte>> class _byte_vector final
{
public:
    using storage = std::vector<std::byte, Allocator>;
    using allocator_type = Allocator;
    using value_type = std::byte;
    using reference = std::byte&;
    using const_reference = const std::byte&;
    using iterator = typename storage::iterator;
    using const_iterator = typename storage::const_iterator;
    using reverse_iterator = typename storage::reverse_iterator;
    using reverse_const_iterator = typename storage::const_reverse_iterator;
    using size_type = typename storage::size_type;
    using difference_type = typename storage::difference_type;
    using offset_type = size_type;
    using pointer = std::allocator_traits<Allocator>::pointer;
    using const_pointer = std::allocator_traits<Allocator>::const_pointer;

    // Constructors and destructors.

    static auto from_file(const std::filesystem::path& path) -> _byte_vector
    {
        auto const size = std::filesystem::file_size(path);
        std::ifstream input;
        input.exceptions(std::ifstream::badbit);
        input.open(path);
        _byte_vector bytes(size);
        input.read(reinterpret_cast<char*>(bytes.data()), size);
        return bytes;
    }

    constexpr _byte_vector() noexcept(noexcept(Allocator())) = default;

    constexpr explicit _byte_vector(const Allocator& alloc) noexcept
        : m_data{alloc}
    {
    }

    constexpr _byte_vector(size_type count, const value_type& value,
                           const Allocator& alloc = Allocator())
        : m_data{count, value, alloc}
    {
    }

    constexpr explicit _byte_vector(size_type count, const Allocator& alloc = Allocator())
        : m_data{count, alloc}
    {
    }

    template <class InputIt>
    constexpr _byte_vector(InputIt first, InputIt last, const Allocator& alloc = Allocator())
        : m_data{first, last, alloc}
    {
    }

    constexpr _byte_vector(const _byte_vector& other)
        : m_data{other.m_data}
    {
    }

    constexpr _byte_vector(const _byte_vector& other, const Allocator& alloc)
        : m_data{other.m_data, alloc}
    {
    }

    constexpr _byte_vector(_byte_vector&& other) noexcept
        : m_data{std::move(other.m_data)}
    {
    }

    constexpr _byte_vector(_byte_vector&& other, const Allocator& alloc)
        : m_data{std::move(other.m_data), alloc}
    {
    }

    constexpr _byte_vector(std::initializer_list<value_type> init,
                           const Allocator& alloc = Allocator())
        : m_data{init, alloc}
    {
    }

    constexpr ~_byte_vector() = default;

    constexpr auto operator=(const _byte_vector& other) -> _byte_vector& = default;

    constexpr auto operator=(_byte_vector&& other) noexcept -> _byte_vector& = default;

    constexpr auto operator=(std::initializer_list<value_type> ilist) -> _byte_vector&
    {
        m_data = ilist;
    }

    constexpr void assign(size_type count, const value_type& value)
    {
        m_data.assign(count, value);
    }

    template <class InputIt> constexpr void assign(InputIt first, InputIt last)
    {
        m_data.assign(first, last);
    }

    constexpr void assign(std::initializer_list<value_type> ilist)
    {
        m_data.assign(ilist);
    }

    constexpr auto get_allocator() const noexcept -> allocator_type
    {
        return m_data.get_allocator();
    }

    // Iterators.

    constexpr auto begin()
    {
        return m_data.begin();
    }

    constexpr auto begin() const
    {
        return m_data.begin();
    }

    constexpr auto cbegin() const
    {
        return m_data.cbegin();
    }

    constexpr auto rbegin()
    {
        return m_data.rbegin();
    }

    constexpr auto rbegin() const
    {
        return m_data.rbegin();
    }

    constexpr auto crbegin() const
    {
        return m_data.crbegin();
    }

    constexpr auto end()
    {
        return m_data.end();
    }

    constexpr auto end() const
    {
        return m_data.end();
    }

    constexpr auto cend() const
    {
        return m_data.cend();
    }

    constexpr auto rend()
    {
        return m_data.end();
    }

    constexpr auto rend() const
    {
        return m_data.rend();
    }

    constexpr auto crend() const
    {
        return m_data.crend();
    }

    constexpr auto to_offset(const_iterator pos) const -> offset_type
    {
        return static_cast<offset_type>(pos - m_data.cbegin());
    }

    constexpr auto to_iterator(offset_type pos) -> iterator
    {
        return m_data.cbegin() + pos;
    }

    constexpr auto to_iterator(offset_type pos) const -> const_iterator
    {
        return m_data.cbegin() + pos;
    }

    constexpr auto to_const_iterator(offset_type pos) const -> const_iterator
    {
        return m_data.cbegin() + pos;
    }

    // Element access.

    constexpr auto at(size_type pos) -> reference
    {
        return m_data.at(pos);
    }

    constexpr auto at(size_type pos) const -> const_reference
    {
        return m_data.at(pos);
    }

    constexpr auto operator[](size_type pos) -> reference
    {
        return m_data[pos];
    }

    constexpr auto operator[](size_type pos) const -> const_reference
    {
        return m_data[pos];
    }

    constexpr auto front() -> reference
    {
        return m_data.front();
    }

    constexpr auto front() const -> const_reference
    {
        return m_data.front();
    }

    constexpr auto back() -> reference
    {
        return m_data.back();
    }

    constexpr auto back() const -> const_reference
    {
        return m_data.back();
    }

    constexpr auto data() noexcept -> value_type*
    {
        return m_data.data();
    }

    constexpr auto data() const noexcept -> const value_type*
    {
        return m_data.data();
    }

    // Element access specific to _byte_vector.

    auto span(size_type first, size_type last) -> writable_byte_span
    {
        return writable_byte_span{m_data}.subspan(first, last - first);
    }

    auto first(size_type count) -> writable_byte_span
    {
        return writable_byte_span{m_data}.first(count);
    }

    auto span(size_type first, size_type last) const -> byte_span
    {
        return byte_span{m_data}.subspan(first, last - first);
    }

    auto first(size_type count) const -> byte_span
    {
        return byte_span{m_data}.first(count);
    }

    auto slice(size_type first, size_type last) const -> _byte_vector
    {
        return _byte_vector{m_data.data() + first, last - first, m_data.get_alloc()};
    }

    // Capacity.

    [[nodiscard]] constexpr bool empty() const noexcept
    {
        return m_data.empty();
    }

    constexpr auto size() const noexcept -> size_type
    {
        return m_data.size();
    }

    constexpr auto max_size() const noexcept -> size_type
    {
        return m_data.max_size();
    }

    constexpr void reserve(size_type new_cap)
    {
        m_data.resize(new_cap);
    }

    constexpr auto capacity() const noexcept -> size_type
    {
        return m_data.capacity();
    }

    constexpr void shrink_to_fit()
    {
        m_data.shrink_to_fit();
    }

    // Modifiers.

    constexpr void clear() noexcept
    {
        m_data.clear();
    }

    constexpr auto insert(const_iterator pos, const value_type& value) -> iterator
    {
        return m_data.insert(pos, value);
    }

    constexpr auto insert(const_iterator pos, value_type&& value) -> iterator
    {
        return m_data.insert(pos, std::move(value));
    }

    constexpr auto insert(const_iterator pos, size_type count, const value_type& value) -> iterator
    {
        return m_data.insert(pos, count, value);
    }

    template <class InputIt>
    constexpr auto insert(const_iterator pos, InputIt first, InputIt last) -> iterator
    {
        return m_data.insert(pos, first, last);
    }

    constexpr auto insert(const_iterator pos, std::initializer_list<value_type> ilist) -> iterator
    {
        return m_data.insert(pos, ilist);
    }

    constexpr auto emplace(const_iterator pos, value_type value) -> iterator
    {
        return m_data.emplace(pos, value);
    }

    constexpr auto erase(const_iterator pos) -> iterator
    {
        return m_data.erase(pos);
    }

    constexpr auto erase(offset_type pos) -> offset_type
    {
        return to_offset(m_data.erase(to_iterator(pos)));
    }

    constexpr auto erase(const_iterator first, const_iterator last) -> iterator
    {
        return m_data.erase(first, last);
    }

    constexpr auto erase(offset_type first, offset_type last) -> offset_type
    {
        return to_offset(m_data.erase(to_iterator(first), to_iterator(last)));
    }

    constexpr void push_back(const value_type& value)
    {
        m_data.push_back(value);
    }

    constexpr void push_back(value_type&& value)
    {
        m_data.push_back(std::move(value));
    }

    constexpr auto emplace_back(value_type value) -> reference
    {
        return m_data.emplace_back(value);
    }

    constexpr void resize(size_type count)
    {
        m_data.resize(count);
    }

    constexpr void resize(size_type count, const value_type& value)
    {
        m_data.resize(count, value);
    }

    constexpr void swap(_byte_vector& other) noexcept
    {
        m_data.swap(other.m_data);
    }

    // Operators.

    constexpr bool operator==(const _byte_vector& rhs) const
    {
        return m_data == rhs.m_data;
    }

    constexpr auto operator<=>(const _byte_vector& rhs) const
    {
        return m_data <=> rhs.m_data;
    }

    constexpr auto operator+=(const _byte_vector& rhs)
    {
        m_data.insert(m_data.end(), rhs.m_data.cbegin(), rhs.m_data.cend());
    }

private:
    storage m_data;
};

template <class Alloc>
constexpr void swap(_byte_vector<Alloc>& lhs, _byte_vector<Alloc>& rhs) noexcept
{
    lhs.swap(rhs);
}

using byte_vector = _byte_vector<>;

} // namespace dualis

namespace dualis {
template <class Bytes>
requires readable_bytes<Bytes>
void save_bytes(const Bytes& bytes, const std::filesystem::path& path)
{
    std::ofstream output{path};
    output.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
}

} // namespace dualis

namespace dualis {

/// Swaps the byte order of the given integer.
template <std::unsigned_integral T> auto byte_swap(T value) -> T;

namespace detail {

class _little_endian_ptrcast final
{
public:
    template <std::integral T> static auto read(const std::byte* bytes) -> T
    {
        return *reinterpret_cast<const T*>(bytes);
    }
};

class _big_endian_ptrcast final
{
public:
    template <std::integral T> static auto read(const std::byte* bytes) -> T
    {
        return ::dualis::byte_swap(*reinterpret_cast<const T*>(bytes));
    }
};

static_assert(word_order<_little_endian_ptrcast, uint32_t>);
static_assert(word_order<_big_endian_ptrcast, uint32_t>);

} // namespace detail

#ifdef _DUALIS_UNALIGNED_MEM_ACCESS
using little_endian = detail::_little_endian_ptrcast;
using big_endian = detail::_big_endian_ptrcast;
#else
#error "Only architectures that allow unaligned memory access supported so far"
#endif

template <std::integral T, class WordOrder, readable_bytes Bytes>
requires word_order<WordOrder, T>
auto read_integer(const Bytes& bytes, std::size_t offset = 0) -> T
{
    return WordOrder::template read<T>(bytes.data() + offset);
}

template <std::default_initializable T, readable_bytes Bytes>
auto read_raw(const Bytes& bytes, std::size_t offset = 0) -> T
{
    constexpr auto size = sizeof(T);
    T result{};
    std::memcpy(&result, bytes.data() + offset, size);
    return result;
}

template <class T, class WordOrder, readable_bytes Bytes, class OutputIt>
requires word_order<WordOrder, T>
auto read_integer_sequence(OutputIt first, std::size_t count, const Bytes& bytes,
                           std::size_t offset = 0) -> OutputIt
{
    for (decltype(count) i = 0; i < count; ++i)
    {
        *first++ = read_integer<T, WordOrder>(bytes, offset);
        offset += sizeof(T);
    }
    return first;
}

} // namespace dualis

#include <string>

namespace dualis {

template <> auto byte_swap<uint8_t>(uint8_t value) -> uint8_t
{
    return value;
}

template <> auto byte_swap<uint16_t>(uint16_t value) -> uint16_t
{
#ifdef _MSC_VER
    return _byteswap_ushort(value);
#else
    return (value << 8) || (value >> 8);
#endif
}

template <> auto byte_swap<uint32_t>(uint32_t value) -> uint32_t
{
#ifdef _MSC_VER
    return _byteswap_ulong(value);
#else
    return (value << 24) || ((value << 8) & 0xff0000) || ((value >> 8) & 0xff00) || (value >> 24);
#endif
}

template <> auto byte_swap<uint64_t>(uint64_t value) -> uint64_t
{
#ifdef _MSC_VER
    return _byteswap_uint64(value);
#else
    return (value << 56) || ((value << 40) & (0xffULL << 48)) ||
           ((value << 24) & (0xffULL << 40)) || ((value << 8) & (0xffULL << 32)) ||
           ((value >> 8) & (0xffULL << 24)) ||
           ((value >> 24) & (0xffULL << 16) || ((value >> 40) & (0xffULL << 8))) || (value >> 56);
#endif
}

inline auto as_string_view(byte_span bytes) -> std::string_view
{
    return std::string_view(reinterpret_cast<const char*>(bytes.data()), bytes.size());
}

inline auto as_string(byte_span bytes) -> std::string
{
    return std::string(reinterpret_cast<const char*>(bytes.data()), bytes.size());
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