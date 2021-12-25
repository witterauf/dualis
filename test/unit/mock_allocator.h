#pragma once

#include <cstddef>
#include <type_traits>

struct allocator_stats
{
    std::size_t copied{0};
    std::size_t moved{0};
    std::size_t allocated{0};
    std::size_t deallocated{0};
    std::size_t selected{0};
};

template <class AlwaysEqual, class Propagate> struct mock_allocator
{
    using value_type = std::byte;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

    using is_always_equal = AlwaysEqual;
    using propagate_on_container_copy_assignment = Propagate;
    using propagate_on_container_move_assignment = Propagate;
    using propagate_on_container_swap = Propagate;

    std::allocator<std::byte> allocator;
    allocator_stats* stats;
    unsigned id{0};

    constexpr mock_allocator() noexcept
        : stats{nullptr}
        , id{0}
    {
    }

    constexpr mock_allocator(allocator_stats* stats, unsigned id = 0)
        : stats{stats}
        , id{id}
    {
    }

    constexpr mock_allocator(const mock_allocator& other) noexcept
        : allocator{other.allocator}
        , stats{other.stats}
        , id{other.id}
    {
        if (stats)
        {
            stats->copied += 1;
        }
    }

    constexpr mock_allocator(mock_allocator&& other) noexcept
        : allocator{std::move(other.allocator)}
        , stats{other.stats}
        , id{other.id}
    {
        if (stats)
        {
            stats->moved += 1;
        }
    }

    constexpr auto operator=(const mock_allocator& other) noexcept -> mock_allocator&
    {
        if (this != std::addressof(other))
        {
            stats = other.stats;
            id = other.id;
            if (stats)
            {
                stats->copied += 1;
            }
        }
        return *this;
    }

    constexpr auto operator=(mock_allocator&& other) noexcept -> mock_allocator&
    {
        if (this != std::addressof(other))
        {
            stats = other.stats;
            id = other.id;
            if (stats)
            {
                stats->moved += 1;
            }
        }
        return *this;
    }

    [[nodiscard]] constexpr std::byte* allocate(std::size_t n)
    {
        if (stats)
        {
            stats->allocated += 1;
        }
        return allocator.allocate(n);
    }

    constexpr void deallocate(std::byte* p, std::size_t n)
    {
        if (stats)
        {
            stats->deallocated += 1;
        }
        allocator.deallocate(p, n);
    }

    [[nodiscard]] constexpr auto select_on_container_copy_construction() const -> mock_allocator
    {
        auto new_alloc = *this;
        if (new_alloc.stats)
        {
            new_alloc.stats->selected += 1;
        }
        return new_alloc;
    }

    [[nodiscard]] constexpr bool operator==(const mock_allocator& rhs) const noexcept
    {
        if (is_always_equal::value)
        {
            return true;
        }
        else
        {
            return id == rhs.id;
        }
    }

    void start_tracking()
    {
        if (stats)
        {
            *stats = {0};
        }
    }
};

using always_equal_allocator = mock_allocator<std::true_type, std::false_type>;
using not_always_equal_allocator = mock_allocator<std::false_type, std::true_type>;
using propagate_allocator = mock_allocator<std::false_type, std::true_type>;
using dont_propagate_allocator = always_equal_allocator;
using propagate_always_equal_allocator = mock_allocator<std::true_type, std::true_type>;
using dont_propagate_not_always_equal_allocator = mock_allocator<std::false_type, std::false_type>;
