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

///////////////////////////////////////////////////////////////////////////////////////////////////
// Concepts
///////////////////////////////////////////////////////////////////////////////////////////////////

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

template<class T, class U>
concept word_order = requires(const std::byte* bytes, std::byte* writableBytes,
                              U value)
{
    { T::template read<U>(bytes) } -> std::same_as<U>;
    T::template write<U>(value, writableBytes);
};
// clang-format on

} // namespace dualis

///////////////////////////////////////////////////////////////////////////////////////////////////
// Containers
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace dualis {

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
    using pointer = typename std::allocator_traits<Allocator>::pointer;
    using const_pointer = typename std::allocator_traits<Allocator>::const_pointer;

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

    auto writable_subspan(size_type first, size_type last) -> writable_byte_span
    {
        return writable_byte_span{m_data}.subspan(first, last - first);
    }

    auto subspan(size_type first, size_type last) const -> byte_span
    {
        return byte_span{m_data}.subspan(first, last - first);
    }

    auto slice(size_type first, size_type last) const -> _byte_vector
    {
        return _byte_vector(m_data.data() + first, m_data.data() + last, m_data.get_allocator());
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

class const_byte_iterator
{
public:
    using iterator_concept = std::contiguous_iterator_tag;
    using iterator_category = std::random_access_iterator_tag;
    using value_type = std::byte;
    using difference_type = std::ptrdiff_t;
    using pointer = const std::byte*;
    using reference = const std::byte&;

    constexpr const_byte_iterator() noexcept = default;

    constexpr const_byte_iterator(const std::byte* pointer) noexcept
        : m_pointer{pointer}
    {
    }

    constexpr auto operator=(const const_byte_iterator& rhs) noexcept
        -> const_byte_iterator& = default;

    [[nodiscard]] constexpr auto operator*() const noexcept -> reference
    {
        return *m_pointer;
    }

    [[nodiscard]] constexpr auto operator->() const noexcept -> pointer
    {
        return m_pointer;
    }

    constexpr auto operator++() noexcept -> const_byte_iterator&
    {
        ++m_pointer;
        return *this;
    }

    constexpr auto operator++(int) noexcept -> const_byte_iterator
    {
        auto const temp = *this;
        ++*this;
        return temp;
    }

    constexpr auto operator--() noexcept -> const_byte_iterator&
    {
        --m_pointer;
        return *this;
    }

    constexpr auto operator--(int) noexcept -> const_byte_iterator
    {
        auto const temp = *this;
        --*this;
        return temp;
    }

    constexpr auto operator+=(const difference_type offset) noexcept -> const_byte_iterator&
    {
        m_pointer += offset;
        return *this;
    }

    [[nodiscard]] constexpr auto operator+(const difference_type offset) noexcept
        -> const_byte_iterator
    {
        auto temp = *this;
        temp += offset;
        return temp;
    }

    constexpr auto operator-=(const difference_type offset) noexcept -> const_byte_iterator&
    {
        m_pointer -= offset;
        return *this;
    }

    [[nodiscard]] constexpr auto operator-(const difference_type offset) noexcept
        -> const_byte_iterator
    {
        auto temp = *this;
        temp -= offset;
        return temp;
    }

    [[nodiscard]] constexpr auto operator[](const difference_type offset) const noexcept
        -> reference
    {
        return m_pointer[offset];
    }

    [[nodiscard]] constexpr bool operator==(const const_byte_iterator& rhs) const noexcept
    {
        return m_pointer == rhs.m_pointer;
    }

    [[nodiscard]] constexpr auto operator<=>(const const_byte_iterator& rhs) const noexcept
        -> std::strong_ordering
    {
        return m_pointer <=> rhs.m_pointer;
    }

private:
    const std::byte* m_pointer{nullptr};
};

class byte_iterator : public const_byte_iterator
{
public:
    using iterator_concept = std::contiguous_iterator_tag;
    using iterator_category = std::random_access_iterator_tag;
    using value_type = std::byte;
    using difference_type = std::ptrdiff_t;
    using pointer = std::byte*;
    using reference = std::byte&;

    using const_byte_iterator::const_byte_iterator;

    [[nodiscard]] constexpr auto operator*() const noexcept -> reference
    {
        return const_cast<reference>(const_byte_iterator::operator*());
    }

    [[nodiscard]] constexpr auto operator->() const noexcept -> pointer
    {
        return const_cast<pointer>(const_byte_iterator::operator->());
    }

    constexpr auto operator++() noexcept -> byte_iterator&
    {
        const_byte_iterator::operator++();
        return *this;
    }

    constexpr auto operator++(int) noexcept -> byte_iterator
    {
        auto const temp = *this;
        const_byte_iterator::operator++();
        return temp;
    }

    constexpr auto operator--() noexcept -> byte_iterator&
    {
        const_byte_iterator::operator--();
        return *this;
    }

    constexpr auto operator--(int) noexcept -> byte_iterator
    {
        auto const temp = *this;
        const_byte_iterator::operator--();
        return temp;
    }

    constexpr auto operator+=(const difference_type offset) noexcept -> byte_iterator&
    {
        const_byte_iterator::operator+=(offset);
        return *this;
    }

    [[nodiscard]] auto operator+(const difference_type offset) const noexcept -> byte_iterator
    {
        auto temp = *this;
        temp += offset;
        return temp;
    }

    constexpr auto operator-=(const difference_type offset) noexcept -> byte_iterator&
    {
        const_byte_iterator::operator-=(offset);
        return *this;
    }

    [[nodiscard]] constexpr auto operator-(const difference_type offset) const noexcept
        -> byte_iterator
    {
        auto temp = *this;
        temp -= offset;
        return temp;
    }

    [[nodiscard]] constexpr auto operator[](const difference_type offset) const noexcept
        -> reference
    {
        return const_cast<reference>(const_byte_iterator::operator[](offset));
    }
};

namespace detail {

// Empty base class optimization: Reduces the storage required if Alloc is an empty class (that is,
// one without member variables). This is required to optimized the necessary storage if an
// allocator for the following containers is empty.
template <class Alloc, class T, bool = std::is_empty_v<Alloc> && !std::is_final_v<T>>
class _allocator_hider final : private Alloc
{
public:
    T data{nullptr};

    constexpr _allocator_hider(const Alloc& alloc)
        : Alloc{alloc}
    {
    }

    constexpr auto allocator() noexcept -> Alloc&
    {
        return *this;
    }

    constexpr auto allocator() const noexcept -> const Alloc&
    {
        return *this;
    }
};

// If the first type (Alloc) is not empty, store both instances as members.
template <class Alloc, class T> class _allocator_hider<Alloc, T, false> final
{
public:
    T data{nullptr};

    constexpr _allocator_hider(const Alloc& alloc)
        : m_allocator{alloc}
    {
    }

    constexpr auto allocator() noexcept -> Alloc&
    {
        return m_allocator;
    }

    constexpr auto allocator() const noexcept -> const Alloc&
    {
        return m_allocator;
    }

private:
    Alloc m_allocator;
};

} // namespace detail

template <class Allocator = std::allocator<std::byte>> class _small_byte_vector
{
public:
    using allocator_type = Allocator;
    using value_type = std::byte;
    using pointer = std::byte*;
    using size_type = typename allocator_type::size_type;
    using difference_type = typename allocator_type::difference_type;
    using iterator = byte_iterator;
    using const_iterator = const_byte_iterator;
    using allocator_traits = std::allocator_traits<allocator_type>;

    constexpr static size_type BufferSize = 16;

    constexpr _small_byte_vector() noexcept(noexcept(Allocator{}))
        : m_allocated{Allocator{}}
    {
    }

    constexpr explicit _small_byte_vector(const Allocator& alloc) noexcept
        : m_allocated{alloc}
    {
    }

    constexpr _small_byte_vector(const size_type count, const value_type& value,
                                 const Allocator& alloc = Allocator{})
        : m_allocated{alloc}
        , m_length{count}
    {
        construct(count);
        std::memset(data(), std::to_integer<uint8_t>(value), count);
    }

    constexpr _small_byte_vector(size_type count, const Allocator& alloc = Allocator{})
        : m_allocated{alloc}
        , m_length{count}
    {
        construct(count);
    }

    constexpr _small_byte_vector(const _small_byte_vector& other)
        : m_allocated{allocator_traits::select_on_container_copy_construction(
              other.get_allocator())}
        , m_length{other.size()}
    {
        if (other.is_allocated())
        {
            m_allocated.data =
                allocator_traits::allocate(m_allocated.allocator(), other.capacity());
            m_data.capacity = other.capacity();
        }
        std::memcpy(data(), other.data(), other.size());
    }

    constexpr _small_byte_vector(const _small_byte_vector& other, const Allocator& alloc)
        : m_allocated{alloc}
        , m_length{other.size()}
    {
        if (other.is_allocated())
        {
            m_allocated.data =
                allocator_traits::allocate(m_allocated.allocator(), other.capacity());
            m_data.capacity = other.capacity();
        }
        std::memcpy(data(), other.data(), other.size());
    }

    constexpr ~_small_byte_vector()
    {
        if (is_allocated())
        {
            allocator_traits::deallocate(m_allocated.allocator(), m_allocated.data,
                                         m_data.capacity);
        }
    }

    constexpr auto get_allocator() const noexcept -> allocator_type
    {
        return m_allocated.allocator();
    }

    [[nodiscard]] constexpr bool empty() const
    {
        return m_length == 0;
    }

    [[nodiscard]] constexpr auto size() const -> size_type
    {
        return m_length;
    }

    [[nodiscard]] constexpr auto max_size() const -> size_type
    {
        return allocator_traits::max_size(get_allocator());
    }

    [[nodiscard]] constexpr auto capacity() const noexcept -> size_type
    {
        return is_allocated() ? m_data.capacity : BufferSize;
    }

    [[nodiscard]] constexpr auto operator[](size_type pos) -> std::byte&
    {
        return data()[pos];
    }

    [[nodiscard]] constexpr auto operator[](size_type pos) const -> std::byte
    {
        return data()[pos];
    }

    [[nodiscard]] constexpr auto data() -> std::byte*
    {
        if (is_allocated())
        {
            return m_allocated.data;
        }
        else
        {
            return m_data.local;
        }
    }

    [[nodiscard]] constexpr auto data() const -> const std::byte*
    {
        return const_cast<_small_byte_vector&>(*this).data();
    }

    [[nodiscard]] constexpr auto begin() -> iterator
    {
        return byte_iterator{data()};
    }

    [[nodiscard]] constexpr auto end() -> iterator
    {
        return begin() + size();
    }

    [[nodiscard]] constexpr auto begin() const -> const_iterator
    {
        return const_byte_iterator{data()};
    }

    [[nodiscard]] constexpr auto end() const -> const_iterator
    {
        return begin() + size();
    }

    [[nodiscard]] constexpr auto cbegin() const -> const_iterator
    {
        return const_byte_iterator{data()};
    }

    [[nodiscard]] constexpr auto cend() const -> const_iterator
    {
        return begin() + size();
    }

private:
    [[nodiscard]] constexpr bool is_allocated() const
    {
        if constexpr (BufferSize > 0)
        {
            return m_allocated.data != nullptr;
        }
        else
        {
            return true;
        }
    }

    constexpr void construct(const size_type size)
    {
        if (size > BufferSize)
        {
            m_allocated.data = allocator_traits::allocate(m_allocated.allocator(), size);
        }
    }

    detail::_allocator_hider<allocator_type, pointer> m_allocated;
    size_type m_length{0};
    union
    {
        std::byte local[BufferSize];
        size_type capacity;
    } m_data;
};

using small_byte_vector = _small_byte_vector<>;

} // namespace dualis

///////////////////////////////////////////////////////////////////////////////////////////////////
// I/O
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace dualis {

template <class Bytes>
requires readable_bytes<Bytes>
void save_bytes(const Bytes& bytes, const std::filesystem::path& path)
{
    std::ofstream output{path};
    output.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
}

} // namespace dualis

///////////////////////////////////////////////////////////////////////////////////////////////////
// Byte interpretation
///////////////////////////////////////////////////////////////////////////////////////////////////

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

    template <std::integral T> static void write(T value, std::byte* bytes)
    {
        *reinterpret_cast<T*>(bytes) = value;
    }
};

class _big_endian_ptrcast final
{
public:
    template <std::integral T> static auto read(const std::byte* bytes) -> T
    {
        return ::dualis::byte_swap(
            static_cast<typename std::make_unsigned<T>::type>(*reinterpret_cast<const T*>(bytes)));
    }

    template <std::integral T> static void write(T value, std::byte* bytes)
    {
        *reinterpret_cast<T*>(bytes) =
            ::dualis::byte_swap(static_cast<typename std::make_unsigned<T>::type>(value));
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

// Read sizeof(T) bytes from bytes and interpret it as an integer of type T using the given
// WordOrder.
template <std::integral T, class WordOrder, readable_bytes Bytes>
requires word_order<WordOrder, T>
auto read_integer(const Bytes& bytes, std::size_t offset = 0) -> T
{
    return WordOrder::template read<T>(bytes.data() + offset);
}

// Read sizeof(T) bytes from bytes and return them as an instance of T.
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

template <std::integral T, class WordOrder, std::integral U, writable_bytes Bytes>
requires word_order<WordOrder, T>
void write_integer(Bytes& bytes, U value, std::size_t offset = 0)
{
    WordOrder::template write<T>(static_cast<T>(value), bytes.data() + offset);
}

} // namespace dualis

///////////////////////////////////////////////////////////////////////////////////////////////////
// Streams
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace dualis {

class byte_reader
{
public:
    byte_reader() = default;

    byte_reader(byte_span bytes) noexcept(noexcept(byte_span{byte_span{}}))
        : m_data{bytes}
    {
    }

    auto tellg() const -> std::size_t
    {
        return m_offset;
    }

    void seekg(std::size_t offset)
    {
        m_offset = offset;
    }

    auto span() const -> byte_span
    {
        return m_data;
    }

    template <std::integral T, class WordOrder>
    requires word_order<WordOrder, T>
    auto consume_integer() -> T
    {
        auto const value = ::dualis::read_integer<T, WordOrder>(m_data, m_offset);
        m_offset += sizeof(T);
        return value;
    }

    template <class T, class WordOrder, class OutputIt>
    requires word_order<WordOrder, T>
    auto consume_integer_sequence(OutputIt first, std::size_t count) -> OutputIt
    {
        auto const after = read_integer_sequence<T, WordOrder>(first, count, m_data, m_offset);
        m_offset += sizeof(T) * count;
        return after;
    }

private:
    byte_span m_data;
    std::size_t m_offset{0};
};

} // namespace dualis

///////////////////////////////////////////////////////////////////////////////////////////////////
// Utilities
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <string>

namespace dualis {

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