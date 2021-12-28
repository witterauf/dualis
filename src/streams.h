#pragma once

#include "containers.h"

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

    template <byte_packing Packing> auto unpack() -> typename Packing::value_type
    {
        auto const value = ::dualis::unpack<Packing>(m_data, m_offset);
        m_offset += Packing::size();
        return value;
    }

    template <byte_packing... Packings>
    auto unpack_sequence() -> std::tuple<typename Packings::value_type...>
    {
        auto const value = ::dualis::unpack_sequence<Packings...>(m_data, m_offset);
        m_offset += sequence<Packings...>::size();
        return value;
    }

    template <byte_packing Packing, class OutputIt>
    auto unpack_n(OutputIt first, std::size_t n) -> OutputIt
    {
        auto const after = ::dualis::unpack_n<Packing>(first, n, m_data, m_offset);
        m_offset += Packing::size() * n;
        return after;
    }

private:
    byte_span m_data;
    std::size_t m_offset{0};
};

} // namespace dualis
