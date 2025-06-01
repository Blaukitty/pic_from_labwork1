/* Kniazeva Eva Andreevna
st128104@student.spbu.ru 
labwork 1 */

#ifndef IMAGE_H
#define IMAGE_H

#include <string>

#pragma pack(push, 1)

struct FileHeader {
    uint16_t type;
    uint32_t file_size;
    uint16_t reversed1;
    uint16_t reversed2;
    uint32_t offset;
};

struct InfoHeader {
    uint32_t header_size;
    int32_t width;
    int32_t height;
    uint16_t planes;
    uint16_t bit_count;
    uint32_t compression;
    uint32_t image_size;
    int32_t x_pxl_per_mtr;
    int32_t y_pxl_per_mtr;
    uint32_t colors_used;
    uint32_t colors_important;
};

struct Pixel {
    uint8_t blue;
    uint8_t green;
    uint8_t red;
};

#pragma pack(pop)

class Image {
public:
    Image();
    ~Image();

    bool readFile(const std::string& path);
    bool writeFile(const std::string& path);
    void clearMemory();
    void rotateClockwise();
    void rotateCounterClockwise();
    void gaussianBlur();

private:
    Pixel **pixels = nullptr;
    FileHeader file_header;
    InfoHeader info_header;
};

#endif
