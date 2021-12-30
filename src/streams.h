#pragma once

#include "containers.h"

namespace dualis {

class byte_stream
{
public:
    byte_stream() = default;

    byte_stream(byte_span bytes) noexcept(noexcept(byte_span{byte_span{}}))
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

    auto span() const -> const byte_span&
    {
        return m_data;
    }

    template <byte_packing Packing> [[nodiscard]] auto unpack() -> typename Packing::value_type
    {
        auto const value = ::dualis::unpack<Packing>(m_data, m_offset);
        m_offset += Packing::size();
        return value;
    }

    template <byte_packing... Packings>
    [[nodiscard]] auto unpack_record() -> std::tuple<typename Packings::value_type...>
    {
        auto const value = ::dualis::unpack_record<Packings...>(m_data, m_offset);
        m_offset += record<Packings...>::size();
        return value;
    }

    template <byte_packing Packing, class OutputIt>
    auto unpack_n(OutputIt first, std::size_t n) -> OutputIt
    {
        auto const after = ::dualis::unpack_n<Packing>(m_data, m_offset, first, n);
        m_offset += Packing::size() * n;
        return after;
    }

private:
    byte_span m_data;
    std::size_t m_offset{0};
};

namespace detail {

template <byte_packing Packing> class _stream_unpacker
{
public:
    using value_type = typename Packing::value_type;

    explicit _stream_unpacker(value_type& output)
        : output{output}
    {
    }

    value_type& output;
};

} // namespace detail

template <byte_packing Packing> auto unpack(typename Packing::value_type& output)
{
    return detail::_stream_unpacker<Packing>{output};
}

template <byte_packing Packing>
auto operator>>(byte_stream& stream, detail::_stream_unpacker<Packing> unpacker) -> byte_stream&
{
    unpacker.output = stream.unpack<Packing>();
    return stream;
}

} // namespace dualis
