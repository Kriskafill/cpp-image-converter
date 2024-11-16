#include "bmp_image.h"
#include "pack_defines.h"

#include <array>
#include <fstream>
#include <string_view>

using namespace std;

namespace img_lib {

    PACKED_STRUCT_BEGIN BitmapFileHeader{
    char signature[2];
    uint32_t fileSize;
    uint32_t reserved;
    uint32_t dataOffset;
    } PACKED_STRUCT_END;

    PACKED_STRUCT_BEGIN BitmapInfoHeader{
        uint32_t size;
        int32_t width;
        int32_t height;
        uint16_t planes;
        uint16_t bpp;
        uint32_t compression;
        uint32_t dataSize;
        int32_t hRes;
        int32_t vRes;
        uint32_t colorsUsed;
        uint32_t importantColors;
    } PACKED_STRUCT_END;
    
    static const int COLORS_COUNT = 3;
    static const int PADDING_MULTIPLIER = 4;

    static int GetBMPStride(int w) {
        return PADDING_MULTIPLIER * ((w * COLORS_COUNT + COLORS_COUNT) / PADDING_MULTIPLIER);
    }

    bool SaveBMP(const Path& file, const Image& image) {
        std::ofstream out(file, std::ios::binary);

        const int w = image.GetWidth();
        const int h = image.GetHeight();

        const int padding = GetBMPStride(w) - (w * 3);
        const int dataSize = (w * 3 + padding) * h;
        const int fileSize = sizeof(BitmapFileHeader) + sizeof(BitmapInfoHeader) + dataSize;

        BitmapFileHeader fileHeader{};
        fileHeader.signature[0] = 'B';
        fileHeader.signature[1] = 'M';
        fileHeader.fileSize = fileSize;
        fileHeader.reserved = 0;
        fileHeader.dataOffset = sizeof(BitmapFileHeader) + sizeof(BitmapInfoHeader);

        BitmapInfoHeader infoHeader{};
        infoHeader.size = sizeof(BitmapInfoHeader);
        infoHeader.width = w;
        infoHeader.height = h;
        infoHeader.planes = 1;
        infoHeader.bpp = 24;
        infoHeader.compression = 0;
        infoHeader.dataSize = dataSize;
        infoHeader.hRes = 11811;
        infoHeader.vRes = 11811;
        infoHeader.colorsUsed = 0;
        infoHeader.importantColors = 0x1000000;

        out.write(reinterpret_cast<const char*>(&fileHeader), sizeof(fileHeader));
        out.write(reinterpret_cast<const char*>(&infoHeader), sizeof(infoHeader));

        std::vector<char> buff(w * 3);

        for (int y = h - 1; y >= 0; --y) {
            const Color* line = image.GetLine(y);
            for (int x = 0; x < w; ++x) {
                buff[x * 3 + 2] = static_cast<char>(line[x].r);
                buff[x * 3 + 1] = static_cast<char>(line[x].g);
                buff[x * 3 + 0] = static_cast<char>(line[x].b);
            }

            out.write(buff.data(), w * 3);

            if (padding > 0) {
                out.write("\0\0\0\0", padding);
            }
        }

        return out.good();
    }

    Image LoadBMP(const Path& file) {
        std::ifstream ifs(file, std::ios::binary);
        if (!ifs) {
            return {};
        }

        BitmapFileHeader fileHeader;
        BitmapInfoHeader infoHeader;

        if (!ifs.read(reinterpret_cast<char*>(&fileHeader), sizeof(fileHeader))) {
            return {};
        }
        if (!ifs.read(reinterpret_cast<char*>(&infoHeader), sizeof(infoHeader))) {
            return {};
        }

        if (fileHeader.signature[0] != 'B' || fileHeader.signature[1] != 'M') {
            return {};
        }

        if (infoHeader.bpp != 24) {
            return {};
        }

        int w = infoHeader.width;
        int h = infoHeader.height;
        int stride = GetBMPStride(w);

        Image result(w, h, Color::Black());
        std::vector<char> buff(stride);

        for (int y = h - 1; y >= 0; --y) {
            Color* line = result.GetLine(y);
            ifs.read(buff.data(), stride);

            for (int x = 0; x < w; ++x) {
                line[x].b = static_cast<byte>(buff[x * 3]);
                line[x].g = static_cast<byte>(buff[x * 3 + 1]);
                line[x].r = static_cast<byte>(buff[x * 3 + 2]);
            }
        }

        return result;
    }

}