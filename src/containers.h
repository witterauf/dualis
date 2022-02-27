#pragma once

#include "concepts.h"
#include "utilities.h"
#include <cstring>
#include <initializer_list>
#include <span>
#include <stdexcept>
#include <string>

namespace dualis {

using writable_byte_span = std::span<std::byte>;
using byte_span = std::span<const std::byte>;

static_assert(byte_range<byte_span>);
static_assert(!writable_byte_range<byte_span>);
static_assert(byte_range<writable_byte_span>);

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

    [[nodiscard]] constexpr auto operator+(difference_type offset) const noexcept
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

    [[nodiscard]] constexpr auto operator-(const difference_type offset) const noexcept
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

inline auto operator+(const_byte_iterator::difference_type diff, const const_byte_iterator iter)
    -> const_byte_iterator
{
    return iter + diff;
}

class byte_iterator final : public const_byte_iterator
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

    using const_byte_iterator::operator-;

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

inline auto operator+(byte_iterator::difference_type diff, byte_iterator iter) -> byte_iterator
{
    return iter += diff;
}

} // namespace dualis

// Usually, it would suffice to provide element_type as a nested alias within the iterator classes.
// Then, the default implementation of std::pointer_traits would deduce all necessary types.
// However, because of a bug in the gcc-10 standard library implementation, this fails because
// there's both a value_type and element_type. This is why std::pointer_traits is explicitly
// specialized here.

template <> struct std::pointer_traits<dualis::const_byte_iterator>
{
    using pointer = dualis::const_byte_iterator;
    using element_type = const dualis::const_byte_iterator::value_type;
    using difference_type = dualis::const_byte_iterator::difference_type;

    static auto to_address(pointer p) noexcept -> element_type*
    {
        return p.operator->();
    }
};

template <> struct std::pointer_traits<dualis::byte_iterator>
{
    using pointer = dualis::byte_iterator;
    using element_type = dualis::byte_iterator::value_type;
    using difference_type = dualis::byte_iterator::difference_type;

    static auto to_address(pointer p) noexcept -> element_type*
    {
        return p.operator->();
    }
};

namespace dualis {

static_assert(std::contiguous_iterator<const_byte_iterator>);
static_assert(std::contiguous_iterator<byte_iterator>);

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

    constexpr void assign(const Alloc& allocator)
    {
        *this = allocator;
    }

    constexpr void assign(Alloc&& allocator)
    {
        *this = std::move(allocator);
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

template <class Allocator, typename Allocator::size_type LocalSize = 16> class _byte_storage final
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

    constexpr void clear()
    {
        m_length = 0;
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
            move_bytes(data() + index + count, data() + index, count);
            inserter(data() + index);
        }
        m_length = new_size;
    }

    template <class Functor>
    constexpr void resize(
        size_type count, Functor f = [](std::byte*, size_type) {})
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

    constexpr void erase(size_type offset, size_type count)
    {
        move_bytes(data() + offset, data() + offset + count, size() - offset - count);
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

    // Also templated on SizeType to make the specialization on N == 0 a partial specialization;
    // otherwise, gcc complains that full specializations are not allowed in class scope.
    // (Although they are, see CWG727---it's an acknowledged bug:
    //  https://gcc.gnu.org/bugzilla/show_bug.cgi?id=85282)
    template <class SizeType, std::size_t N> union compressed_t
    {
        SizeType capacity;
        std::byte local[N];
    };
    // If BufferSize == 0 then don't even provide the array m_data.local to force compiler
    // errors in case any method accesses m_data.local although it shouldn't. (Note: this is
    // possible because of "if constexpr".)
    template <class SizeType> union compressed_t<SizeType, 0>
    {
        SizeType capacity;
    };

    static_assert(sizeof(compressed_t<size_type, 0>) == sizeof(size_type));
    static_assert(sizeof(compressed_t<size_type, sizeof(size_type) * 2>) == sizeof(size_type) * 2);

    compressed_t<size_type, BufferSize> m_data{0}; // initializes capacity to 0
};

} // namespace detail

template <class Allocator = std::allocator<std::byte>,
          typename Allocator::size_type EmbeddedSize = sizeof(typename Allocator::size_type) * 2>
class byte_container
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

    constexpr byte_container() noexcept(noexcept(allocator_type{}))
        : m_storage{allocator_type{}}
    {
    }

    constexpr explicit byte_container(const allocator_type& allocator) noexcept
        : m_storage{allocator}
    {
    }

    constexpr byte_container(const size_type count, const value_type& value,
                             const allocator_type& allocator = allocator_type{})
        : m_storage{count, allocator}
    {
        std::memset(m_storage.data(), std::to_integer<uint8_t>(value), count);
    }

    constexpr byte_container(const std::byte* bytes, size_type count,
                             const allocator_type& allocator = allocator_type{})
        : m_storage{count, allocator}
    {
        copy_bytes(m_storage.data(), bytes, count);
    }

    constexpr byte_container(size_type count, const allocator_type& allocator = allocator_type{})
        : m_storage{count, allocator}
    {
    }

    constexpr byte_container(const byte_container& other)
        : m_storage{other.m_storage}
    {
    }

    constexpr byte_container(const byte_container& other, const allocator_type& allocator)
        : m_storage{other.m_storage, allocator}
    {
    }

    constexpr byte_container(byte_container&& other) noexcept
        : m_storage{std::move(other.m_storage)}
    {
    }

    constexpr byte_container(byte_container&& other, const allocator_type& allocator)
        : m_storage{std::move(other.m_storage), allocator}
    {
    }

    constexpr byte_container(std::initializer_list<std::byte> init,
                             const allocator_type& allocator = allocator_type{})
        : m_storage{init.size(), allocator}
    {
        copy_bytes(data(), init.begin(), init.size());
    }

    template <std::contiguous_iterator Iterator>
    constexpr byte_container(Iterator begin, Iterator end,
                             const allocator_type& allocator = allocator_type{})
        : m_storage{narrow_cast<size_type>(end - begin), allocator}
    {
        copy_bytes(m_storage.data(), std::to_address(begin), end - begin);
    }

    template <std::random_access_iterator Iterator>
    constexpr byte_container(Iterator begin, Iterator end,
                             const allocator_type& allocator = allocator_type{})
        : m_storage{end - begin, allocator}
    {
        while (begin != end)
        {
            push_back(*begin++);
        }
    }

    template <std::input_iterator Iterator>
    constexpr byte_container(Iterator begin, Iterator end,
                             const allocator_type& allocator = allocator_type{})
        : m_storage{allocator}
    {
        while (begin != end)
        {
            push_back(*begin++);
        }
    }

    template <byte_range Bytes>
    explicit constexpr byte_container(const Bytes& bytes,
                                      const allocator_type allocator = allocator_type{})
        : m_storage{std::ranges::size(bytes), allocator}
    {
        copy_bytes(m_storage.data(), std::ranges::cdata(bytes), bytes.size());
    }

    //== assignment ===============================================================================

    constexpr auto operator=(const byte_container& rhs) -> byte_container&
    {
        return assign(rhs);
    }

    constexpr auto operator=(byte_container&& rhs) -> byte_container&
    {
        return assign(std::move(rhs));
    }

    constexpr auto operator=(std::initializer_list<std::byte> ilist) -> byte_container&
    {
        return assign(ilist);
    }

    constexpr auto operator=(std::byte value) -> byte_container&
    {
        return assign(1, value);
    }

    constexpr auto assign(const byte_container& other) -> byte_container&
    {
        if (this != std::addressof(other))
        {
            m_storage.assign(other.m_storage);
        }
        return *this;
    }

    constexpr auto assign(byte_container&& other) -> byte_container&
    {
        if (this != std::addressof(other))
        {
            m_storage.assign(std::move(other.m_storage));
        }
        return *this;
    }

    constexpr auto assign(size_type count, std::byte value) -> byte_container&
    {
        m_storage.reassign(count);
        std::memset(m_storage.data(), std::to_integer<int>(value), count);
        return *this;
    }

    constexpr auto assign(const std::byte* bytes, const size_type count) -> byte_container&
    {
        m_storage.assign(bytes, count);
        return *this;
    }

    constexpr auto assign(std::initializer_list<std::byte> ilist) -> byte_container&
    {
        m_storage.assign(ilist.begin(), ilist.size());
        return *this;
    }

    template <std::contiguous_iterator Iterator>
    constexpr auto assign(Iterator begin, Iterator end) -> byte_container&
    {
        m_storage.assign(std::to_address(begin), end - begin);
        return *this;
    }

    template <std::input_iterator Iterator>
    constexpr auto assign(Iterator begin, Iterator end) -> byte_container&
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

    //=============================================================================================

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

    //== iterators ================================================================================

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

    constexpr auto insert(size_type offset, size_type count, std::byte value) -> byte_container&
    {
        m_storage.insert(offset, count,
                         [count, value](std::byte* data) { set_bytes(data, value, count); });
        return *this;
    }

    constexpr auto insert(size_type offset, const std::byte* bytes, size_type count)
        -> byte_container&
    {
        m_storage.insert(offset, count,
                         [bytes, count](std::byte* data) { copy_bytes(data, bytes, count); });
        return *this;
    }

    template <byte_range Bytes>
    constexpr auto insert(size_type offset, const Bytes& bytes) -> byte_container&
    {
        m_storage.insert(offset, std::ranges::size(bytes), [&bytes](std::byte* data) {
            copy_bytes(data, std::ranges::cdata(bytes), std::ranges::size(bytes));
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
        -> byte_container&
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

    template <byte_packing Packing>
    constexpr auto insert_packed(size_type offset, const typename Packing::value_type& value)
        -> byte_container&
    {
        m_storage.insert(offset, Packing::size(),
                         [value](std::byte* data) { Packing::pack(data, value); });
        return *this;
    }

    template <byte_packing... Packings>
    constexpr auto insert_packed_tuple(size_type offset,
                                       const typename Packings::value_type&... values)
        -> byte_container&;

    template <byte_packing Packing, std::ranges::sized_range Range>
    constexpr auto insert_packed_range(size_type offset, const Range& range) -> byte_container&;

    //== erase() ==================================================================================

    constexpr auto erase(size_type offset = 0, size_type count = npos) -> byte_container&
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

    constexpr auto append(size_type count, std::byte value) -> byte_container&
    {
        m_storage.append(count, [value, count](std::byte* dest) {
            std::memset(dest, std::to_integer<int>(value), count);
        });
        return *this;
    }

    constexpr auto append(const std::byte* bytes, size_type count) -> byte_container&
    {
        m_storage.append(count,
                         [bytes, count](std::byte* dest) { copy_bytes(dest, bytes, count); });
        return *this;
    }

    constexpr auto append(std::initializer_list<std::byte> ilist) -> byte_container&
    {
        return append(ilist.begin(), ilist.end() - ilist.begin());
    }

    template <std::contiguous_iterator Iterator>
    constexpr auto append(Iterator first, Iterator last) -> byte_container&
    {
        const std::byte* bytes = std::to_address(first);
        auto const count = last - first;
        return append(bytes, count);
    }

    template <std::input_iterator Iterator>
    constexpr auto append(Iterator first, Iterator last) -> byte_container&
    {
        while (first != last)
        {
            push_back(*first++);
        }
        return *this;
    }

    template <byte_range Bytes> constexpr auto append(const Bytes& bytes) -> byte_container&
    {
        return append(std::ranges::cdata(bytes), std::ranges::size(bytes));
    }

    template <byte_packing Packing>
    constexpr auto append_packed(const typename Packing::value_type& value) -> byte_container&
    {
        m_storage.append(Packing::size(), [value](std::byte* dest) { Packing::pack(dest, value); });
        return *this;
    }

    // Convenience method for appending packed tuples. This overload is declared for at least two
    // values because otherwise, append_packed would become ambiguous for one value.
    template <byte_packing Packing, byte_packing Packing2, byte_packing... Packings>
    constexpr auto append_packed(const typename Packing::value_type& value,
                                 const typename Packing2::value_type& value2,
                                 const typename Packings::value_type&... values) -> byte_container&
    {
        return append_packed_tuple<Packing, Packing2, Packings...>(value, value2, values...);
    }

    template <byte_packing... Packings>
    constexpr auto append_packed_tuple(const typename Packings::value_type&... values)
        -> byte_container&;

    // Requires a different method name than append_packed because otherwise, it might become
    // ambiguous in case Packing::value_type is a range.
    template <byte_packing Packing, std::ranges::sized_range Range>
    constexpr auto append_packed_range(const Range& range) -> byte_container&;

    constexpr auto operator+=(std::byte value) -> byte_container&
    {
        push_back(value);
        return *this;
    }

    constexpr auto operator+=(std::initializer_list<std::byte> ilist) -> byte_container&
    {
        return append(ilist);
    }

    template <byte_range Bytes> constexpr auto operator+=(const Bytes& bytes) -> byte_container&
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
        m_storage.resize(m_storage.size() - 1, [](std::byte*, size_type) {});
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

    constexpr void swap(byte_container& other) noexcept
    {
        if (this != std::addressof(other))
        {
            m_storage.swap(other.m_storage);
        }
    }

    [[nodiscard]] constexpr bool operator==(const byte_container& rhs) const noexcept
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

    [[nodiscard]] constexpr bool operator!=(const byte_container& rhs) const noexcept
    {
        return !this->operator==(rhs);
    }

    [[nodiscard]] constexpr auto operator<=>(const byte_container& rhs) const noexcept
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

    //== sub-ranges ===============================================================================

    auto extract(size_type offset = 0, size_type count = npos) const -> byte_container
    {
        auto const actual_count = count == npos ? size() - offset : count;
        return byte_container{m_storage.data() + offset, actual_count};
    }

    auto subspan(size_type offset = 0, size_type count = npos) const -> byte_span
    {
        auto const actual_count = count == npos ? size() - offset : count;
        return byte_span{m_storage.data() + offset, actual_count};
    }

    auto writable_subspan(size_type offset = 0, size_type count = npos) -> writable_byte_span
    {
        auto const actual_count = count == npos ? size() - offset : count;
        return writable_byte_span{m_storage.data() + offset, actual_count};
    }

private:
    detail::_byte_storage<allocator_type, EmbeddedSize> m_storage;
};

template <class Alloc>
auto operator+(byte_container<Alloc> lhs, const byte_container<Alloc>& rhs) -> byte_container<Alloc>
{
    return lhs += rhs;
}

template <class Alloc>
auto operator+(byte_container<Alloc> lhs, std::byte rhs) -> byte_container<Alloc>
{
    return lhs += rhs;
}

template <class Alloc>
auto operator+(std::byte lhs, byte_container<Alloc> rhs) -> byte_container<Alloc>
{
    return rhs += lhs;
}

template <class Allocator, typename Allocator::size_type EmbeddedSize>
void swap(byte_container<Allocator, EmbeddedSize>& a, byte_container<Allocator, EmbeddedSize>& b)
{
    a.swap(b);
}

using byte_string = byte_container<>;
using byte_vector = byte_container<std::allocator<std::byte>, 0>;

static_assert(writable_byte_range<byte_string>);
static_assert(writable_byte_range<byte_vector>);

inline auto as_string_view(byte_span bytes) -> std::string_view
{
    return std::string_view(reinterpret_cast<const char*>(bytes.data()), bytes.size());
}

inline auto as_string(byte_span bytes) -> std::string
{
    return std::string(reinterpret_cast<const char*>(bytes.data()), bytes.size());
}

} // namespace dualis

#include <filesystem>
#include <fstream>

namespace dualis {

template <byte_range Bytes> void save_bytes(const Bytes& bytes, const std::filesystem::path& path)
{
    std::ofstream output{path};
    output.write(reinterpret_cast<const char*>(std::ranges::cdata(bytes)),
                 std::ranges::size(bytes));
}

template <size_constructible_bytes Bytes>
auto load_bytes(const std::filesystem::path& path) -> Bytes
{
    auto const size = std::filesystem::file_size(path);
    std::ifstream input{path};
    Bytes bytes(size);
    input.read(reinterpret_cast<char*>(bytes.data()), size);
    return bytes;
}

} // namespace dualis
