namespace literals {

inline auto operator"" _bspan(const char* ascii, std::size_t length) -> byte_span
{
    return byte_span{reinterpret_cast<const std::byte*>(ascii), length};
}

}

namespace detail {

template <class T> auto ceil_divide(T a, std::type_identity_t<T> b) -> std::pair<T, T>
{
    return std::make_pair(a / b + (a % b ? 1 : 0), a % b);
}

template <class T, std::random_access_iterator OutputIterator>
void to_hex(OutputIterator begin, OutputIterator end, T value)
{
    auto const length = std::distance(begin, end);
    for (size_t i; begin != end; ++begin, ++i)
    {
        auto const digit = (value >> ((length - i - 1) * 4)) & 0xf;
        *begin = detail::HexDigits[digit];
    }
}

} // namespace detail

template <class LineConsumer>
void hex_dump(byte_span data, LineConsumer&& consume, size_t columns = 16, size_t start_address = 0)
{
    auto const address_width =
        detail::ceil_divide(std::bit_width(start_address + data.size()), 4).first;
    auto const hex_width = columns * 3 - 1;
    auto const ascii_width = columns;
    auto const separator_width = 3;

    std::string line(address_width + hex_width + ascii_width + separator_width * 2, ' ');
    for (size_t offset{0}; offset < data.size(); offset += columns)
    {
        auto const address = start_address + offset;
        detail::to_hex(line.begin(), line.begin() + address_width, address);

        auto hex_pos = address_width + separator_width;
        auto ascii_pos = hex_pos + hex_width + separator_width;
        for (size_t i{0}; i < columns && offset + i < data.size(); ++i, hex_pos += 3, ascii_pos += 1)
        {
            auto const value = static_cast<char>(data[offset + i]);
            line[hex_pos] = detail::HexDigits[(value >> 4) & 0xf];
            line[hex_pos + 1] = detail::HexDigits[value & 0xf];
            if (value >= 0x20)
            {
                line[ascii_pos] = value;
            }
        }

        consume(line);
    }
}
