#include "bmp_image.h"
#include "pack_defines.h"

#include <array>
#include <fstream>
#include <string_view>

using namespace std;

namespace img_lib {

    PACKED_STRUCT_BEGIN BitmapFileHeader{
    char signature[2]; // Подпись "BM"
    uint32_t fileSize; // Размер файла
    uint32_t reserved; // Зарезервировано
    uint32_t dataOffset; // Смещение до данных
    } PACKED_STRUCT_END;

    PACKED_STRUCT_BEGIN BitmapInfoHeader{
        uint32_t size; // Размер структуры заголовка
        int32_t width; // Ширина в пикселях
        int32_t height; // Высота в пикселях
        uint16_t planes; // Количество цветовых плоскостей
        uint16_t bpp; // Количество бит на пиксель
        uint32_t compression; // Тип сжатия
        uint32_t dataSize; // Размер данных
        int32_t hRes; // Горизонтальное разрешение
        int32_t vRes; // Вертикальное разрешение
        uint32_t colorsUsed; // Количество использованных цветов
        uint32_t importantColors; // Количество значимых цветов
    } PACKED_STRUCT_END;
    
    static const int COLORS_COUNT = 3;
    static const int PADDING_MULTIPLIER = 4;

    // Функция вычисления отступа по ширине
    static int GetBMPStride(int w) {
        return PADDING_MULTIPLIER * ((w * COLORS_COUNT + COLORS_COUNT) / PADDING_MULTIPLIER);
    }

    bool SaveBMP(const Path& file, const Image& image) {
        std::ofstream out(file, std::ios::binary);

        const int w = image.GetWidth();
        const int h = image.GetHeight();

        const int padding = GetBMPStride(w) - (w * 3); // Вычисляем отступ для выравнивания на 4 байта
        const int dataSize = (w * 3 + padding) * h; // Размер данных
        const int fileSize = sizeof(BitmapFileHeader) + sizeof(BitmapInfoHeader) + dataSize; // Общий размер файла

        // Заполняем заголовки
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

        // Запись заголовков в файл
        out.write(reinterpret_cast<const char*>(&fileHeader), sizeof(fileHeader));
        out.write(reinterpret_cast<const char*>(&infoHeader), sizeof(infoHeader));

        std::vector<char> buff(w * 3); // Размер буфера для данных пикселей без отступа

        for (int y = h - 1; y >= 0; --y) {
            const Color* line = image.GetLine(y);
            for (int x = 0; x < w; ++x) {
                buff[x * 3 + 2] = static_cast<char>(line[x].r); // Красный
                buff[x * 3 + 1] = static_cast<char>(line[x].g); // Зеленый
                buff[x * 3 + 0] = static_cast<char>(line[x].b); // Синий
            }

            // Запись строки данных
            out.write(buff.data(), w * 3);

            // Добавляем отступ
            if (padding > 0) {
                out.write("\0\0\0\0", padding); // Пишем отступ, если он необходим
            }
        }

        return out.good();
    }

    // напишите эту функцию
    Image LoadBMP(const Path& file) {
        std::ifstream ifs(file, std::ios::binary);
        if (!ifs) {
            return {}; // Не удалось открыть файл
        }

        BitmapFileHeader fileHeader;
        BitmapInfoHeader infoHeader;

        // Читаем заголовки BMP
        if (!ifs.read(reinterpret_cast<char*>(&fileHeader), sizeof(fileHeader))) {
            return {};
        }
        if (!ifs.read(reinterpret_cast<char*>(&infoHeader), sizeof(infoHeader))) {
            return {};
        }

        // Проверка заголовков
        if (fileHeader.signature[0] != 'B' || fileHeader.signature[1] != 'M') {
            return {}; // Неверная подпись файла
        }

        // Проверка на 24-битное изображение
        if (infoHeader.bpp != 24) {
            return {}; // Поддерживается только 24-битное изображение
        }

        // Вычисляем строки
        int w = infoHeader.width;
        int h = infoHeader.height;
        int stride = GetBMPStride(w);

        Image result(w, h, Color::Black());
        std::vector<char> buff(stride);

        // Чтение данных построчно
        for (int y = h - 1; y >= 0; --y) {
            Color* line = result.GetLine(y);
            ifs.read(buff.data(), stride);

            // Копируем 24-битные цвета
            for (int x = 0; x < w; ++x) {
                line[x].b = static_cast<byte>(buff[x * 3]);     // Синий
                line[x].g = static_cast<byte>(buff[x * 3 + 1]); // Зеленый
                line[x].r = static_cast<byte>(buff[x * 3 + 2]); // Красный
            }
        }

        return result;
    }

}  // namespace img_lib