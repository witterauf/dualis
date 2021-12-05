#include <dualis.h>
#include <iostream>

using namespace dualis;

auto readHeader(dualis::byte_span bmp) -> std::pair<size_t, size_t>
{
    using namespace dualis;
    auto const size = read_integer<uint16_t, little_endian>(bmp, 2);
    auto const offset = read_integer<uint32_t, little_endian>(bmp, 10);
    return std::make_pair(size, offset);
}

struct BitmapInfoHeader
{
    uint32_t biSize{0};
    int32_t biWidth{0}, biHeight{0};
    uint16_t biPlanes{1};
    uint16_t biBitCount{32};
    uint32_t biCompression{0};
    uint32_t biImageSize{0};
    int32_t biXPelsPerMeter{0}, biYPelsPerMeter{0};
    uint32_t biClrUsed{0};
    uint32_t biClrImportant{0};
};

static_assert(sizeof(BitmapInfoHeader) == 40);

auto readInfoHeader(byte_reader& reader) -> BitmapInfoHeader
{
    BitmapInfoHeader infoHeader;
    infoHeader.biSize = reader.consume_integer<uint32_t, little_endian>();
    infoHeader.biWidth = reader.consume_integer<int32_t, little_endian>();
    infoHeader.biHeight = reader.consume_integer<int32_t, little_endian>();
    infoHeader.biPlanes = reader.consume_integer<uint16_t, little_endian>();
    infoHeader.biBitCount = reader.consume_integer<uint16_t, little_endian>();
    infoHeader.biCompression = reader.consume_integer<uint32_t, little_endian>();
    infoHeader.biImageSize = reader.consume_integer<uint32_t, little_endian>();
    infoHeader.biXPelsPerMeter = reader.consume_integer<int32_t, little_endian>();
    infoHeader.biYPelsPerMeter = reader.consume_integer<int32_t, little_endian>();
    infoHeader.biClrUsed = reader.consume_integer<uint32_t, little_endian>();
    infoHeader.biClrImportant = reader.consume_integer<uint32_t, little_endian>();
    return infoHeader;
}

void printInfoHeader(const BitmapInfoHeader& infoHeader)
{
    std::cout << "Dimensions: " << std::abs(infoHeader.biWidth) << "x"
              << std::abs(infoHeader.biHeight) << " @ " << infoHeader.biBitCount << "bpp\n";
}

void printPalette(const std::vector<uint32_t>& palette)
{
    if (!palette.empty())
    {
        std::cout << "This is an indexed bitmap:\n";
        for (unsigned i = 0; i < palette.size(); ++i)
        {
            std::cout << " Color " << i << ": " << palette[i] << "\n";
        }
    }
}

void printBitmapInfo(const std::filesystem::path& path)
{
    // byte_vector owns the memory.
    auto const bmp = dualis::byte_vector::from_file(path);
    // byte_span does not own the memory---it's merely a convenient view.
    auto const bmpSpan = dualis::byte_span{bmp};

    if (as_string_view(bmpSpan.first(2)) != "BM")
    {
        std::cerr << "Error: the given file is not a valid BMP file (wrong magic number)\n";
        std::exit(EXIT_FAILURE);
    }

    auto const [size, offset] = readHeader(bmpSpan);

    byte_reader reader{bmpSpan};
    reader.seekg(14);
    auto const infoHeaderA = readInfoHeader(reader);
    // Only works on little-endian machines! Avoid if possible.
    auto const infoHeaderB = read_raw<BitmapInfoHeader>(bmpSpan, 14);

    if (std::memcmp(&infoHeaderA, &infoHeaderB, sizeof(BitmapInfoHeader)) != 0)
    {
        std::cerr << "Warning: read mismatch\n";
    }

    std::vector<uint32_t> palette;
    if (infoHeaderA.biBitCount <= 8)
    {
        auto const count =
            infoHeaderA.biClrUsed == 0 ? (1 << infoHeaderA.biBitCount) : infoHeaderA.biClrUsed;
        palette.resize(count);
        reader.consume_integer_sequence<uint32_t, little_endian>(palette.begin(), count);
    }

    printInfoHeader(infoHeaderA);
    printPalette(palette);
}

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        std::cout << "Usage: " << argv[0] << " PATH_TO_BMP\n";
        std::exit(EXIT_SUCCESS);
    }
    const std::filesystem::path path{argv[1]};

    try
    {
        printBitmapInfo(path);
    }
    catch (std::exception& e)
    {
        std::cerr << "Error: " << e.what() << "\n";
        std::exit(EXIT_FAILURE);
    }
}
