template <class Allocator, typename Allocator::size_type EmbeddedSize>
template <byte_packing Packing, std::ranges::sized_range Range>
constexpr auto _byte_vector_base<Allocator, EmbeddedSize>::insert_packed_range(size_type offset,
                                                                               const Range& range)
    -> _byte_vector_base&
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
constexpr auto _byte_vector_base<Allocator, EmbeddedSize>::append_packed_range(const Range& range)
    -> _byte_vector_base&
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
