template <class Allocator, typename Allocator::size_type EmbeddedSize>
template <byte_packing Packing, std::ranges::sized_range Range>
constexpr auto byte_container<Allocator, EmbeddedSize>::insert_packed_range(size_type offset,
                                                                            const Range& range)
    -> byte_container&
{
    m_storage.insert(offset, Packing::size() * std::ranges::size(range), [&range](std::byte* dest) {
        for (auto const& value : range)
        {
            Packing::pack(dest, value);
            dest += Packing::size();
        }
    });
    return *this;
}

template <class Allocator, typename Allocator::size_type EmbeddedSize>
template <byte_packing Packing, std::ranges::sized_range Range>
constexpr auto byte_container<Allocator, EmbeddedSize>::append_packed_range(const Range& range)
    -> byte_container&
{
    const auto size = Packing::size() * std::ranges::size(range);
    m_storage.append(size, [&range](std::byte* dest) {
        for (auto const& value : range)
        {
            Packing::pack(dest, value);
            dest += Packing::size();
        }
    });
    return *this;
}

template <class Allocator, typename Allocator::size_type EmbeddedSize>
template <byte_packing... Packings>
constexpr auto byte_container<Allocator, EmbeddedSize>::insert_packed_tuple(
    size_type offset, const typename Packings::value_type&... values) -> byte_container&
{
    m_storage.insert(offset, tuple_packing<Packings...>::size(), [&values...](std::byte* dest) {
        tuple_packing<Packings...>::pack(dest, values...);
    });
    return *this;
}

template <class Allocator, typename Allocator::size_type EmbeddedSize>
template <byte_packing... Packings>
constexpr auto byte_container<Allocator, EmbeddedSize>::append_packed_tuple(
    const typename Packings::value_type&... values) -> byte_container&
{
    m_storage.append(tuple_packing<Packings...>::size(), [&values...](std::byte* dest) {
        tuple_packing<Packings...>::pack(dest, values...);
    });
    return *this;
}

template <byte_range LHS, byte_range RHS> bool operator==(const LHS& lhs, const RHS& rhs)
{
    if (lhs.size() != rhs.size())
    {
        return false;
    }
    return compare_bytes(lhs.data(), rhs.data(), lhs.size()) == 0;
}

template <byte_range LHS, byte_range RHS> bool operator!=(const LHS& lhs, const RHS& rhs)
{
    return !(lhs == rhs);
}

template <byte_range LHS, byte_range RHS>
auto operator<=>(const LHS& lhs, const RHS& rhs) -> std::strong_ordering
{
    auto const lexicographic =
        compare_bytes(lhs.data(), rhs.data(), std::min(lhs.size(), rhs.size()));
    if (lexicographic != 0)
    {
        return lexicographic > 0 ? std::strong_ordering::greater : std::strong_ordering::less;
    }
    else
    {
        return lhs.size() <=> rhs.size();
    }
}
