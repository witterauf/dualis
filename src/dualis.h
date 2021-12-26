#pragma once

#include <concepts>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <iterator>
#include <span>
#include <vector>

// TODO: configure defines using compiler macros?
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

    [[nodiscard]] constexpr auto operator-(const_byte_iterator rhs) const -> difference_type
    {
        return m_pointer - rhs.m_pointer;
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

    using const_byte_iterator::operator-;

    [[nodiscard]] constexpr auto operator[](const difference_type offset) const noexcept
        -> reference
    {
        return const_cast<reference>(const_byte_iterator::operator[](offset));
    }
};

// Use compiler-defined intrinsics if available.
inline auto copy_bytes(std::byte* dest, const std::byte* src, std::size_t size)
{
    std::memcpy(dest, src, size);
}

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

    constexpr _allocator_hider(Alloc&& alloc) noexcept
        : Alloc{std::move(alloc)}
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

    constexpr void swap(_allocator_hider& other) noexcept
    {
        if constexpr (std::allocator_traits<Alloc>::propagate_on_container_swap::value)
        {
            std::swap(*this, other);
        }
        std::swap(data, other.data);
    }
};

// If the first type (Alloc) is not empty, store both instances as members.
template <class Alloc, class T> class _allocator_hider<Alloc, T, false> final
{
public:
    using allocator_traits = typename std::allocator_traits<Alloc>;

    T data{nullptr};

    constexpr _allocator_hider(const Alloc& alloc)
        : m_allocator{alloc}
    {
    }

    constexpr _allocator_hider(Alloc&& alloc) noexcept
        : m_allocator{std::move(alloc)}
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

    constexpr void assign(const Alloc& allocator)
    {
        m_allocator = allocator;
    }

    constexpr void assign(Alloc&& allocator)
    {
        m_allocator = std::move(allocator);
    }

    constexpr void swap(_allocator_hider& other) noexcept
    {
        if constexpr (std::allocator_traits<Alloc>::propagate_on_container_swap::value)
        {
            std::swap(m_allocator, other.m_allocator);
        }
        // It is undefined behavior if the allocator is both not propagated on swap and the two
        // allocators are not equal. It is there "safe" to always swap data.
        std::swap(data, other.data);
    }

private:
    Alloc m_allocator;
};

// Ensure 0-size allocators actually do not result in storage.
static_assert(sizeof(_allocator_hider<std::allocator<std::byte>, std::byte*>) ==
              sizeof(std::byte*));

template <class Allocator, Allocator::size_type LocalSize = 16> class _byte_storage final
{
public:
    using allocator_type = Allocator;
    using allocator_traits = std::allocator_traits<allocator_type>;
    using value_type = std::byte;
    using pointer = std::byte*;
    using size_type = typename allocator_type::size_type;
    using difference_type = typename allocator_type::difference_type;

    static_assert(
        LocalSize == 0 || LocalSize >= sizeof(size_type),
        "LocalSize must either be 0 (disable small object optimization) or >= sizeof(size_type)");
    static constexpr auto BufferSize = LocalSize;

    [[nodiscard]] static constexpr bool is_embedded_enabled()
    {
        return BufferSize > 0;
    }

    constexpr _byte_storage(const allocator_type& allocator) noexcept
        : m_allocated{allocator}
    {
    }

    constexpr _byte_storage(size_type size, const allocator_type& allocator) noexcept
        : m_allocated{allocator}
        , m_length{size}
    {
        if (size > capacity())
        {
            m_allocated.data = allocator_traits::allocate(get_allocator(), size);
            m_data.capacity = size;
        }
    }

    constexpr _byte_storage(const _byte_storage& other, const allocator_type& allocator)
        : m_allocated{allocator}
        , m_length{other.m_length}
    {
        if (other.is_allocated())
        {
            m_allocated.data = allocator_traits::allocate(get_allocator(), other.capacity());
            m_data.capacity = other.capacity();
        }
        copy_bytes(data(), other.data(), other.size());
    }

    constexpr _byte_storage(const _byte_storage& other)
        : _byte_storage{
              other, allocator_traits::select_on_container_copy_construction(other.get_allocator())}
    {
    }

    constexpr _byte_storage(_byte_storage&& other, const allocator_type& allocator)
        : m_allocated{allocator}
        , m_length{other.m_length}
    {
        if (other.is_allocated())
        {
            if constexpr (allocator_traits::is_always_equal::value)
            {
                m_allocated.data = other.m_allocated.data;
                m_data.capacity = other.m_data.capacity;
            }
            else
            {
                if (other.get_allocator() != get_allocator())
                {
                    m_allocated.data =
                        allocator_traits::allocate(get_allocator(), other.capacity());
                    m_data.capacity = other.capacity();
                    copy_bytes(data(), other.data(), other.size());
                    allocator_traits::deallocate(other.get_allocator(), other.m_allocated.data,
                                                 other.capacity());
                }
                else
                {
                    m_allocated.data = other.m_allocated.data;
                    m_data.capacity = other.m_data.capacity;
                }
            }
            other.m_allocated.data = nullptr;
            other.m_data.capacity = 0;
        }
        else
        {
            if constexpr (is_embedded_enabled())
            {
                copy_bytes(m_data.local, other.m_data.local, size());
            }
        }
        other.m_length = 0;
    }

    constexpr _byte_storage(_byte_storage&& other) noexcept
        : m_allocated{std::move(other.get_allocator())}
        , m_length{other.m_length}
    {
        m_allocated.data = other.m_allocated.data;
        if (other.is_allocated())
        {
            m_data.capacity = other.m_data.capacity;
            other.m_allocated.data = nullptr;
            other.m_data.capacity = 0;
        }
        else
        {
            if constexpr (is_embedded_enabled())
            {
                copy_bytes(m_data.local, other.m_data.local, size());
            }
        }
        other.m_length = 0;
    }

    constexpr ~_byte_storage() noexcept
    {
        _destroy();
    }

    constexpr void assign(const _byte_storage& rhs)
    {
        if constexpr (allocator_traits::propagate_on_container_copy_assignment::value)
        {
            change_allocator(rhs.get_allocator(), rhs.size() > capacity());
        }
        assign(rhs.data(), rhs.size());
    }

    constexpr void change_allocator(const allocator_type& allocator, bool destroy_if)
    {
        if constexpr (allocator_traits::is_always_equal::value)
        {
            if (destroy_if)
            {
                _destroy();
            }
        }
        else
        {
            if (destroy_if || get_allocator() != allocator)
            {
                _destroy();
            }
        }
        m_allocated.assign(allocator);
    }

    constexpr void take_contents(_byte_storage& rhs)
    {
        // Expects(get_allocator() == rhs.get_allocator());
        // Expects(is_allocated() == false);
        m_length = rhs.m_length;
        if (rhs.is_allocated())
        {
            m_allocated.data = rhs.m_allocated.data;
            m_data.capacity = rhs.capacity();
        }
        else
        {
            m_allocated.data = nullptr;
            if constexpr (is_embedded_enabled())
            {
                copy_bytes(m_data.local, rhs.m_data.local, rhs.size());
            }
        }
        rhs.m_allocated.data = nullptr;
        rhs.m_length = 0;
    }

    constexpr void assign(_byte_storage&& rhs)
    {
        if constexpr (!allocator_traits::propagate_on_container_move_assignment::value &&
                      !allocator_traits::is_always_equal::value)
        {
            if (get_allocator() != rhs.get_allocator())
            {
                // Don't propagate allocator and allocators not equal means the data needs to be
                // copied.
                assign(rhs.data(), rhs.size());
                rhs.m_length = 0;
                return;
            }
        }

        _destroy();
        if constexpr (allocator_traits::propagate_on_container_move_assignment::value)
        {
            m_allocated.assign(std::move(rhs.m_allocated.allocator()));
        }
        take_contents(rhs);
    }

    constexpr void assign(const std::byte* bytes, size_type count)
    {
        auto* reassigned_data = reassign(count);
        copy_bytes(reassigned_data, bytes, count);
    }

    constexpr void swap(_byte_storage& other) noexcept
    {
        m_allocated.swap(other.m_allocated);
        std::swap(m_length, other.m_length);
        if constexpr (is_embedded_enabled())
        {
            std::byte temp[BufferSize];
            copy_bytes(temp, m_data.local, BufferSize);
            copy_bytes(m_data.local, other.m_data.local, BufferSize);
            copy_bytes(other.m_data.local, temp, BufferSize);
            return;
        }
        else
        {
            std::swap(m_data.capacity, other.m_data.capacity);
        }
    }

    // Resizes the storage without initializing its content and returns the pointer to the new
    // storage.
    constexpr auto reassign(size_type count) -> std::byte*
    {
        m_length = count;
        if (count > capacity())
        {
            auto const new_capacity = _compute_spare_capacity(
                count, capacity(), allocator_traits::max_size(get_allocator()));
            _reallocate(new_capacity, get_allocator());
            return m_allocated.data;
        }
        else
        {
            return data();
        }
    }

    constexpr void reserve(size_type count)
    {
        if (count > capacity())
        {
            auto const new_capacity = _compute_spare_capacity(
                count, capacity(), allocator_traits::max_size(get_allocator()));
            _reallocate(new_capacity, get_allocator());
        }
    }

    constexpr void shrink_to_fit()
    {
        if (is_allocated() && capacity() > size())
        {
            auto* new_data = allocator_traits::allocate(get_allocator(), size());
            copy_bytes(new_data, m_allocated.data, size());
            allocator_traits::deallocate(get_allocator(), m_allocated.data, capacity());
            m_allocated.data = new_data;
            m_data.local = size();
        }
    }

    [[nodiscard]] constexpr auto get_allocator() noexcept -> allocator_type&
    {
        return m_allocated.allocator();
    }

    [[nodiscard]] constexpr auto get_allocator() const noexcept -> const allocator_type&
    {
        return m_allocated.allocator();
    }

    [[nodiscard]] constexpr auto size() const noexcept -> size_type
    {
        return m_length;
    }

    [[nodiscard]] constexpr auto capacity() const noexcept -> size_type
    {
        if constexpr (is_embedded_enabled())
        {
            return is_allocated() ? m_data.capacity : BufferSize;
        }
        else
        {
            return m_data.capacity;
        }
    }

    [[nodiscard]] constexpr bool is_allocated() const noexcept
    {
        return m_allocated.data != nullptr;
    }

    [[nodiscard]] constexpr auto data() -> std::byte*
    {
        if constexpr (is_embedded_enabled())
        {
            return is_allocated() ? m_allocated.data : m_data.local;
        }
        else
        {
            return m_allocated.data;
        }
    }

    [[nodiscard]] constexpr auto data() const -> const std::byte*
    {
        return const_cast<_byte_storage*>(this)->data();
    }

    [[nodiscard]] constexpr auto max_size() const -> size_type
    {
        return allocator_traits::max_size(get_allocator());
    }

    template <class Inserter>
    constexpr void insert(size_type index, size_type count, Inserter inserter)
    {
        auto const new_size = size() + count;
        if (new_size > capacity())
        {
            auto const new_capacity =
                _compute_spare_capacity(new_size, m_data.capacity, max_size());
            auto* new_data = allocator_traits::allocate(get_allocator(), new_capacity);
            copy_bytes(new_data, m_allocated.data, index);
            inserter(new_data + index);
            copy_bytes(new_data + index + count, m_allocated.data + index, size() - index);
            _replace_data(new_data, new_capacity);
        }
        else
        {
            copy_bytes(data() + index + count, data() + index, count);
            inserter(data() + index);
        }
        m_length = new_size;
    }

    template <class Functor> constexpr void resize(size_type count, Functor f)
    {
        if (count > capacity())
        {
            auto* new_data = allocator_traits::allocate(get_allocator(), count);
            if (is_allocated())
            {
                copy_bytes(new_data, m_allocated.data, size());
                allocator_traits::deallocate(get_allocator(), m_allocated.data, m_data.capacity);
            }
            else
            {
                if constexpr (is_embedded_enabled())
                {
                    copy_bytes(new_data, m_data.local, size());
                }
            }
            m_allocated.data = new_data;
            m_data.capacity = count;
        }
        if (count > m_length)
        {
            f(data(), count - m_length);
        }
        m_length = count;
    }

    constexpr void erase(size_type index, size_type count)
    {
        copy_bytes(data() + index + count, data() + index, count);
        m_length -= count;
    }

    template <class Appender> constexpr void append(size_type count, Appender appender)
    {
        auto const new_size = size() + count;
        if (new_size > capacity())
        {
            auto const new_capacity = _compute_spare_capacity(new_size, capacity(), max_size());
            auto* new_data = allocator_traits::allocate(m_allocated.allocator(), new_capacity);
            copy_bytes(new_data, data(), size());
            _replace_data(new_data, new_capacity);
        }
        appender(data() + size());
        m_length = new_size;
    }

private:
    constexpr void _replace_data(std::byte* new_data, size_type new_capacity)
    {
        if (is_allocated())
        {
            allocator_traits::deallocate(get_allocator(), m_allocated.data, m_data.capacity);
        }
        m_allocated.data = new_data;
        m_data.capacity = new_capacity;
    }

    constexpr void _destroy()
    {
        if (is_allocated())
        {
            allocator_traits::deallocate(get_allocator(), m_allocated.data, m_data.capacity);
            m_allocated.data = nullptr;
        }
    }

    constexpr void _reallocate(size_type new_capacity, allocator_type& allocator)
    {
        _destroy();
        m_allocated.data = allocator_traits::allocate(allocator, new_capacity);
        m_data.capacity = new_capacity;
    }

    [[nodiscard]] static constexpr auto _compute_spare_capacity(size_type requested, size_type old,
                                                                size_type max)
    {
        return std::min(std::max(requested, old + old / 2), max);
    }

    detail::_allocator_hider<allocator_type, pointer> m_allocated;
    size_type m_length{0};

    template <size_type N> union compressed_t
    {
        size_type capacity;
        std::byte local[N];
    };
    // If BufferSize == 0 then don't even provide the array m_data.local to force compiler
    // errors in case any method accesses m_data.local although it shouldn't. (Note: this is
    // possible because of "if constexpr".)
    template <> union compressed_t<0>
    {
        size_type capacity;
    };

    static_assert(sizeof(compressed_t<0>) == sizeof(size_type));
    static_assert(sizeof(compressed_t<sizeof(size_type) * 2>) == sizeof(size_type) * 2);

    compressed_t<BufferSize> m_data{0}; // initializes capacity to 0
};

} // namespace detail

template <class Allocator = std::allocator<std::byte>,
          typename Allocator::size_type EmbeddedSize = sizeof(typename Allocator::size_type) * 2>
class _byte_vector_base
{
public:
    using allocator_type = Allocator;
    using allocator_traits = std::allocator_traits<allocator_type>;
    using value_type = std::byte;
    using pointer = std::byte*;
    using size_type = typename allocator_type::size_type;
    using difference_type = typename allocator_type::difference_type;
    using iterator = byte_iterator;
    using const_iterator = const_byte_iterator;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    static constexpr auto npos{static_cast<size_type>(-1)};

    //== constructors =============================================================================

    constexpr _byte_vector_base() noexcept(noexcept(allocator_type{}))
        : m_storage{allocator_type{}}
    {
    }

    constexpr explicit _byte_vector_base(const allocator_type& allocator) noexcept
        : m_storage{allocator}
    {
    }

    constexpr _byte_vector_base(const size_type count, const value_type& value,
                                const allocator_type& allocator = allocator_type{})
        : m_storage{count, allocator}
    {
        std::memset(m_storage.data(), std::to_integer<uint8_t>(value), count);
    }

    constexpr _byte_vector_base(const std::byte* bytes, size_type count,
                                const allocator_type& allocator = allocator_type{})
        : m_storage{count, allocator}
    {
        copy_bytes(m_storage.data(), bytes, count);
    }

    constexpr _byte_vector_base(size_type count, const allocator_type& allocator = allocator_type{})
        : m_storage{count, allocator}
    {
    }

    constexpr _byte_vector_base(const _byte_vector_base& other)
        : m_storage{other.m_storage}
    {
    }

    constexpr _byte_vector_base(const _byte_vector_base& other, const allocator_type& allocator)
        : m_storage{other.m_storage, allocator}
    {
    }

    constexpr _byte_vector_base(_byte_vector_base&& other) noexcept
        : m_storage{std::move(other.m_storage)}
    {
    }

    constexpr _byte_vector_base(_byte_vector_base&& other, const allocator_type& allocator)
        : m_storage{std::move(other.m_storage), allocator}
    {
    }

    constexpr _byte_vector_base(std::initializer_list<std::byte> init,
                                const allocator_type& allocator = allocator_type{})
        : m_storage{init.size(), allocator}
    {
        copy_bytes(data(), init.begin(), init.size());
    }

    template <std::contiguous_iterator Iterator>
    constexpr _byte_vector_base(Iterator begin, Iterator end,
                                const allocator_type& allocator = allocator_type{})
        : m_storage{end - begin, allocator}
    {
        copy_bytes(m_storage.data(), std::to_address(begin), end - begin);
    }

    template <std::random_access_iterator Iterator>
    constexpr _byte_vector_base(Iterator begin, Iterator end,
                                const allocator_type& allocator = allocator_type{})
        : m_storage{end - begin, allocator}
    {
        while (begin != end)
        {
            push_back(*begin++);
        }
    }

    template <std::input_iterator Iterator>
    constexpr _byte_vector_base(Iterator begin, Iterator end,
                                const allocator_type& allocator = allocator_type{})
        : m_storage{allocator}
    {
        while (begin != end)
        {
            push_back(*begin++);
        }
    }

    template <readable_bytes Bytes>
    explicit constexpr _byte_vector_base(const Bytes& bytes,
                                         const allocator_type allocator = allocator_type{})
        : m_storage{bytes.size(), allocator}
    {
        copy_bytes(m_storage.data(), bytes.data(), bytes.size());
    }

    //== assignment ===============================================================================

    constexpr auto operator=(const _byte_vector_base& rhs) -> _byte_vector_base&
    {
        return assign(rhs);
    }

    constexpr auto operator=(_byte_vector_base&& rhs) -> _byte_vector_base&
    {
        return assign(std::move(rhs));
    }

    constexpr auto operator=(std::initializer_list<std::byte> ilist) -> _byte_vector_base&
    {
        return assign(ilist);
    }

    constexpr auto operator=(std::byte value) -> _byte_vector_base&
    {
        return assign(1, value);
    }

    constexpr auto assign(const _byte_vector_base& other) -> _byte_vector_base&
    {
        if (this != std::addressof(other))
        {
            m_storage.assign(other.m_storage);
        }
        return *this;
    }

    constexpr auto assign(_byte_vector_base&& other) -> _byte_vector_base&
    {
        if (this != std::addressof(other))
        {
            m_storage.assign(std::move(other.m_storage));
        }
        return *this;
    }

    constexpr auto assign(size_type count, std::byte value) -> _byte_vector_base&
    {
        m_storage.reassign(count);
        std::memset(m_storage.data(), std::to_integer<int>(value), count);
        return *this;
    }

    constexpr auto assign(const std::byte* bytes, const size_type count) -> _byte_vector_base&
    {
        m_storage.assign(bytes, count);
        return *this;
    }

    constexpr auto assign(std::initializer_list<std::byte> ilist) -> _byte_vector_base&
    {
        m_storage.assign(ilist.begin(), ilist.size());
        return *this;
    }

    template <std::contiguous_iterator Iterator>
    constexpr auto assign(Iterator begin, Iterator end) -> _byte_vector_base&
    {
        m_storage.assign(std::to_address(begin), end - begin);
        return *this;
    }

    template <std::input_iterator Iterator>
    constexpr auto assign(Iterator begin, Iterator end) -> _byte_vector_base&
    {
        clear();
        while (begin != end)
        {
            push_back(*begin++);
        }
        return *this;
    }

    //=============================================================================================

    constexpr auto get_allocator() const noexcept -> allocator_type
    {
        return m_storage.get_allocator();
    }

    [[nodiscard]] constexpr bool empty() const
    {
        return m_storage.size() == 0;
    }

    [[nodiscard]] constexpr auto size() const -> size_type
    {
        return m_storage.size();
    }

    [[nodiscard]] constexpr auto max_size() const -> size_type
    {
        return m_storage.max_size();
    }

    [[nodiscard]] constexpr auto capacity() const noexcept -> size_type
    {
        return m_storage.capacity();
    }

    constexpr void reserve(size_type new_cap)
    {
        if (new_cap >= max_size())
        {
            throw std::length_error{"new capacity exceeds max_size"};
        }
        m_storage.reserve(new_cap);
    }

    constexpr void shrink_to_fit()
    {
        m_storage.shrink_to_fit();
    }

    [[nodiscard]] constexpr auto operator[](size_type pos) -> std::byte&
    {
        return m_storage.data()[pos];
    }

    [[nodiscard]] constexpr auto operator[](size_type pos) const -> std::byte
    {
        return m_storage.data()[pos];
    }

    [[nodiscard]] constexpr auto at(size_type pos) -> std::byte&
    {
        return m_storage.data()[pos];
    }

    [[nodiscard]] constexpr auto at(size_type pos) const -> std::byte
    {
        return m_storage.data()[pos];
    }

    [[nodiscard]] constexpr auto front() -> std::byte&
    {
        return m_storage.data()[0];
    }

    [[nodiscard]] constexpr auto front() const -> std::byte
    {
        return m_storage.data()[0];
    }

    [[nodiscard]] constexpr auto back() -> std::byte&
    {
        return m_storage.data()[m_storage.size() - 1];
    }

    [[nodiscard]] constexpr auto back() const -> std::byte
    {
        return m_storage.data()[m_storage.size() - 1];
    }

    [[nodiscard]] constexpr auto data() -> std::byte*
    {
        return m_storage.data();
    }

    [[nodiscard]] constexpr auto data() const -> const std::byte*
    {
        return m_storage.data();
    }

    [[nodiscard]] constexpr auto begin() noexcept -> iterator
    {
        return byte_iterator{data()};
    }

    [[nodiscard]] constexpr auto end() noexcept -> iterator
    {
        return begin() + size();
    }

    [[nodiscard]] constexpr auto begin() const noexcept -> const_iterator
    {
        return cbegin();
    }

    [[nodiscard]] constexpr auto end() const noexcept -> const_iterator
    {
        return cend();
    }

    [[nodiscard]] constexpr auto cbegin() const noexcept -> const_iterator
    {
        return const_byte_iterator{data()};
    }

    [[nodiscard]] constexpr auto cend() const noexcept -> const_iterator
    {
        return begin() + size();
    }

    [[nodiscard]] constexpr auto rbegin() noexcept -> reverse_iterator
    {
        return reverse_iterator{end()};
    }

    [[nodiscard]] constexpr auto rend() noexcept -> reverse_iterator
    {
        return reverse_iterator{begin()};
    }

    [[nodiscard]] constexpr auto rbegin() const noexcept -> const_reverse_iterator
    {
        return crbegin();
    }

    [[nodiscard]] constexpr auto rend() const noexcept -> const_reverse_iterator
    {
        return crend();
    }

    [[nodiscard]] constexpr auto crbegin() const noexcept -> const_reverse_iterator
    {
        return const_reverse_iterator{cend()};
    }

    [[nodiscard]] constexpr auto crend() const noexcept -> const_reverse_iterator
    {
        return const_reverse_iterator{cbegin()};
    }

    constexpr void clear() noexcept
    {
        m_storage.clear();
    }

    //== insert() =================================================================================

    constexpr auto insert(size_type offset, size_type count, std::byte value) -> _byte_vector_base&
    {
        m_storage.insert(offset, count, [count, value](std::byte* data) {
            std::memset(data, std::to_integer<int>(value), count);
        });
        return *this;
    }

    constexpr auto insert(size_type offset, const std::byte* bytes, size_type count)
        -> _byte_vector_base&
    {
        m_storage.insert(offset, count,
                         [bytes, count](std::byte* data) { std::memcpy(data, bytes, count); });
        return *this;
    }

    template <readable_bytes Bytes>
    constexpr auto insert(size_type offset, const Bytes& bytes) -> _byte_vector_base&
    {
        m_storage.insert(offset, bytes.size(), [&bytes](std::byte* data) {
            std::memcpy(data, bytes.data(), bytes.size());
        });
        return *this;
    }

    constexpr auto insert(const_iterator pos, std::byte value) -> iterator
    {
        auto const index = pos - cbegin();
        insert(index, 1, value);
        return begin() + index;
    }

    constexpr auto insert(const_iterator pos, size_type count, std::byte value) -> iterator
    {
        auto const index = pos - cbegin();
        insert(index, count, value);
        return begin() + index;
    }

    template <std::contiguous_iterator Iterator>
    constexpr auto insert(size_type offset, Iterator first, Iterator last) -> iterator
    {
        const std::byte* bytes = std::to_address(first);
        auto const count = last - first;
        insert(offset, bytes, count);
        return *this;
    }

    template <std::random_access_iterator Iterator>
    constexpr auto insert(size_type offset, Iterator first, Iterator last) -> iterator
    {
        auto const count = last - first;
        m_storage.insert(offset, count, [first, last](std::byte* data) {
            while (first != last)
            {
                *data++ = *first++;
            }
        });
        return *this;
    }

    template <std::input_iterator Iterator>
    constexpr auto insert(size_type offset, Iterator first, Iterator last) -> iterator
    {
        while (first != last)
        {
            insert(offset++, *first);
        }
        return *this;
    }

    template <std::input_iterator Iterator>
    constexpr auto insert(const_iterator pos, Iterator first, Iterator last) -> iterator
    {
        auto const index = pos - cbegin();
        insert(index, first, last);
        return begin() + index;
    }

    constexpr auto insert(size_type offset, std::initializer_list<std::byte> ilist)
        -> _byte_vector_base&
    {
        m_storage.reserve(size() + ilist.size());
        return insert(offset, ilist.begin(), ilist.end());
    }

    constexpr auto insert(const_iterator pos, std::initializer_list<std::byte> ilist) -> iterator
    {
        auto const index = pos - cbegin();
        insert(index, ilist);
        return begin() + index;
    }

    //== erase() ==================================================================================

    constexpr auto erase(size_type offset = 0, size_type count = npos) -> _byte_vector_base&
    {
        m_storage.erase(offset, count == npos ? size() - offset : count);
        return *this;
    }

    constexpr auto erase(const_iterator position) -> iterator
    {
        auto const index = position - cbegin();
        erase(index, 1);
        return begin() + index;
    }

    constexpr auto erase(const_iterator first, const_iterator last) -> iterator
    {
        auto const index = first - cbegin();
        auto const count = last - first;
        erase(index, count);
        return begin() + index;
    }

    //== append() / operator+= ====================================================================

    constexpr auto append(size_type count, std::byte value) -> _byte_vector_base&
    {
        m_storage.append(count, [value, count](std::byte* dest) {
            std::memset(dest, std::to_integer<int>(value), count);
        });
        return *this;
    }

    constexpr auto append(const std::byte* bytes, size_type count) -> _byte_vector_base&
    {
        m_storage.append(count,
                         [bytes, count](std::byte* dest) { copy_bytes(dest, bytes, count); });
        return *this;
    }

    constexpr auto append(std::initializer_list<std::byte> ilist) -> _byte_vector_base&
    {
        return append(ilist.begin(), ilist.end() - ilist.begin());
    }

    template <std::contiguous_iterator Iterator>
    constexpr auto append(Iterator first, Iterator last) -> _byte_vector_base&
    {
        const std::byte* bytes = std::to_address(first);
        auto const count = last - first;
        return append(bytes, count);
    }

    template <std::input_iterator Iterator>
    constexpr auto append(Iterator first, Iterator last) -> _byte_vector_base&
    {
        while (first != last)
        {
            push_back(*first++);
        }
        return *this;
    }

    template <readable_bytes Bytes> constexpr auto append(const Bytes& bytes) -> _byte_vector_base&
    {
        return append(bytes.data(), bytes.size());
    }

    constexpr auto operator+=(std::byte value) -> _byte_vector_base&
    {
        push_back(value);
        return *this;
    }

    constexpr auto operator+=(std::initializer_list<std::byte> ilist) -> _byte_vector_base&
    {
        return append(ilist);
    }

    template <readable_bytes Bytes>
    constexpr auto operator+=(const Bytes& bytes) -> _byte_vector_base&
    {
        return append(bytes);
    }

    //=============================================================================================

    constexpr void push_back(std::byte value)
    {
        m_storage.append(1, [value](std::byte* dest) { *dest = value; });
    }

    constexpr void pop_back()
    {
        m_storage.resize(m_storage.size() - 1);
    }

    constexpr void resize(size_type count)
    {
        m_storage.resize(count, [](std::byte*, size_type) {});
    }

    constexpr void resize(size_type count, std::byte value)
    {
        m_storage.resize(count, [value](std::byte* data, size_type count) {
            std::memset(data, std::to_integer<int>(value), count);
        });
    }

    constexpr void swap(_byte_vector_base& other) noexcept
    {
        if (this != std::addressof(other))
        {
            m_storage.swap(other.m_storage);
        }
    }

    [[nodiscard]] constexpr bool operator==(const _byte_vector_base& rhs) const noexcept
    {
        if (size() != rhs.size())
        {
            return false;
        }
        else
        {
            return std::memcmp(data(), rhs.data(), size()) == 0;
        }
    }

    [[nodiscard]] constexpr bool operator!=(const _byte_vector_base& rhs) const noexcept
    {
        return !this->operator==(rhs);
    }

    [[nodiscard]] constexpr auto operator<=>(const _byte_vector_base& rhs) const noexcept
        -> std::strong_ordering
    {
        auto const lexicographic = std::memcmp(data(), rhs.data(), std::min(size(), rhs.size()));
        if (lexicographic != 0)
        {
            return lexicographic > 0 ? std::strong_ordering::greater : std::strong_ordering::less;
        }
        else
        {
            return size() <=> rhs.size();
        }
    }

    operator byte_span() const noexcept
    {
        return byte_span{m_storage.data(), m_storage.size()};
    }

    operator writable_byte_span() noexcept
    {
        return writable_byte_span{m_storage.data(), m_storage.size()};
    }

private:
    detail::_byte_storage<allocator_type, EmbeddedSize> m_storage;
};

template <class Alloc>
auto operator+(_byte_vector_base<Alloc> lhs, const _byte_vector_base<Alloc>& rhs)
    -> _byte_vector_base<Alloc>
{
    return lhs += rhs;
}

template <class Alloc>
auto operator+(_byte_vector_base<Alloc> lhs, std::byte rhs) -> _byte_vector_base<Alloc>
{
    return lhs += rhs;
}

template <class Alloc>
auto operator+(std::byte lhs, _byte_vector_base<Alloc> rhs) -> _byte_vector_base<Alloc>
{
    return rhs += lhs;
}

template <class Allocator, typename Allocator::size_type EmbeddedSize>
void swap(_byte_vector_base<Allocator, EmbeddedSize>& a,
          _byte_vector_base<Allocator, EmbeddedSize>& b)
{
    a.swap(b);
}

using small_byte_vector = _byte_vector_base<>;

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