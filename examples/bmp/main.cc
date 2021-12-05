#include <dualis.h>
#include <iomanip>
#include <iostream>

using namespace dualis;

enum class BitmapCompression : uint32_t
{
    RGB = 0,
    RLE4 = 1,
    RLE8 = 2,
    BitFields = 3,
};

struct BitmapInfoHeader
{
    uint32_t biSize{0};
    int32_t biWidth{0}, biHeight{0};
    uint16_t biPlanes{1};
    uint16_t biBitCount{32};
    BitmapCompression biCompression{BitmapCompression::RGB};
    uint32_t biImageSize{0};
    int32_t biXPelsPerMeter{0}, biYPelsPerMeter{0};
    uint32_t biClrUsed{0};
    uint32_t biClrImportant{0};

    bool isIndexed() const
    {
        return biBitCount <= 8;
    }

    auto paletteSize() const -> std::size_t
    {
        if (isIndexed())
        {
            return static_cast<std::size_t>(biClrUsed == 0 ? 1 << biBitCount : biClrUsed);
        }
        else
        {
            return 0;
        }
    }
};

static_assert(sizeof(BitmapInfoHeader) == 40);

void checkAgainstRawRead(byte_span bmp, const BitmapInfoHeader& infoHeader)
{
    // Only works on little-endian machines! Avoid if possible.
    auto const infoHeaderB = read_raw<BitmapInfoHeader>(bmp, 14);
    if (std::memcmp(&infoHeader, &infoHeaderB, sizeof(BitmapInfoHeader)) != 0)
    {
        std::cerr << "Warning: read mismatch\n";
    }
}

auto readHeader(dualis::byte_span bmp) -> std::pair<size_t, size_t>
{
    using namespace dualis;
    auto const size = read_integer<uint16_t, little_endian>(bmp, 2);
    auto const offset = read_integer<uint32_t, little_endian>(bmp, 10);
    return std::make_pair(size, offset);
}

auto readInfoHeader(byte_reader& reader) -> BitmapInfoHeader
{
    BitmapInfoHeader infoHeader;
    infoHeader.biSize = reader.consume_integer<uint32_t, little_endian>();
    infoHeader.biWidth = reader.consume_integer<int32_t, little_endian>();
    infoHeader.biHeight = reader.consume_integer<int32_t, little_endian>();
    infoHeader.biPlanes = reader.consume_integer<uint16_t, little_endian>();
    infoHeader.biBitCount = reader.consume_integer<uint16_t, little_endian>();
    infoHeader.biCompression =
        static_cast<BitmapCompression>(reader.consume_integer<uint32_t, little_endian>());
    infoHeader.biImageSize = reader.consume_integer<uint32_t, little_endian>();
    infoHeader.biXPelsPerMeter = reader.consume_integer<int32_t, little_endian>();
    infoHeader.biYPelsPerMeter = reader.consume_integer<int32_t, little_endian>();
    infoHeader.biClrUsed = reader.consume_integer<uint32_t, little_endian>();
    infoHeader.biClrImportant = reader.consume_integer<uint32_t, little_endian>();
    checkAgainstRawRead(reader.span(), infoHeader);
    return infoHeader;
}

auto readPalette(byte_reader& reader, const BitmapInfoHeader& infoHeader) -> std::vector<uint32_t>
{
    std::vector<uint32_t> palette;
    if (infoHeader.paletteSize() > 0)
    {
        palette.resize(infoHeader.paletteSize());
        reader.consume_integer_sequence<uint32_t, little_endian>(palette.begin(),
                                                                 infoHeader.paletteSize());
    }
    return palette;
}

void printInfoHeader(const BitmapInfoHeader& infoHeader)
{
    std::cout << "This bitmap is " << std::abs(infoHeader.biWidth) << "x"
              << std::abs(infoHeader.biHeight) << " @ " << infoHeader.biBitCount << "bpp.\n";
    std::size_t size{infoHeader.biImageSize};
    switch (infoHeader.biCompression)
    {
    case BitmapCompression::RGB:
        std::cout << "It is not compressed";
        if (size == 0)
        {
            size = infoHeader.biWidth * infoHeader.biHeight * infoHeader.biBitCount / 8;
        }
        break;
    case BitmapCompression::RLE4: std::cout << "It is compressed using RLE4"; break;
    case BitmapCompression::RLE8: std::cout << "It is compressed using RLE8"; break;
    case BitmapCompression::BitFields: std::cout << "It is compressed using BitFields"; break;
    default: std::cout << "It is using an unknown or invalid compression"; break;
    }
    std::cout << ", taking up " << size << " bytes.\n";
}

void printPalette(const std::vector<uint32_t>& palette)
{
    if (!palette.empty())
    {
        std::cout << "It is indexed with the following palette:\n";
        for (unsigned i = 0; i < palette.size();)
        {
            std::cout << "  ";
            for (unsigned j = 0; j < 8 && i < palette.size(); ++i, ++j)
            {
                if (j > 0)
                {
                    std::cout << " ";
                }
                std::cout << "[" << std::setw(3) << std::setfill(' ') << std::dec << i << "] ";
                std::cout << "#" << std::setfill('0') << std::hex;
                std::cout << std::setw(2) << ((palette[i] >> 16) & 0xff) << std::setw(2)
                          << ((palette[i] >> 8) & 0xff) << std::setw(2)
                          << ((palette[i] >> 0) & 0xff);
            }
            std::cout << "\n";
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
    auto const infoHeader = readInfoHeader(reader);
    auto const palette = readPalette(reader, infoHeader);
    printInfoHeader(infoHeader);
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
