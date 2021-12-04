#include <dualis.h>
#include <iostream>

auto readHeader(dualis::byte_span bmp) -> std::pair<size_t, size_t>
{
    using namespace dualis;
    auto const size = read_integer<uint16_t, little_endian>(bmp, 2);
    auto const offset = read_integer<uint32_t, big_endian>(bmp, 10);
    return std::make_pair(size, offset);
}

int main(int argc, char* argv[])
{
    using namespace dualis;
    using namespace dualis::literals;

    auto const bmp = dualis::byte_vector::from_file(argv[0]);
    auto const bmpSpan = dualis::byte_span{bmp};

    if (as_string_view(bmpSpan.first(2)) != "BM")
    {
        std::cerr << "Error: the given file is not a valid BMP file.\n";
        std::exit(EXIT_FAILURE);
    }

    auto const [size, offset] = readHeader(bmpSpan);

    std::cout << size << ", " << offset << "\n";
}