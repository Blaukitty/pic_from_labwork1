#include "Image.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <thread>

Image::Image(): pixels(nullptr) {}

Image::~Image() {
    clearMemory();
}

bool Image::readFile(const std::string& path) {
    if (pixels != nullptr) {
        clearMemory();
    }

    std::ifstream file(path, std::ios::binary);
    if (!file) {
        std::cout << "Failed to open the file." << std::endl;
        return false;
    }

    file.read(reinterpret_cast<char*>(&file_header), sizeof(file_header));
    if (file_header.type != 0x4D42) {
        std::cout << "Not a BMP file." << std::endl;
        return false;
    }

    file.read(reinterpret_cast<char*>(&info_header), sizeof(info_header));

    int width = info_header.width;
    int height = std::abs(info_header.height);
    int padding = (4 - ((width*3) % 4)) % 4;

    pixels = new Pixel*[height];
    for (int i = 0; i < height; ++i) {
        pixels[i] = new Pixel[width];
    }

    file.seekg(file_header.offset, std::ios::beg);

    for (int i = height - 1; i >= 0; --i) {
        for(int j = 0; j < width; ++j) {
            file.read(reinterpret_cast<char*>(&pixels[i][j]), 3);
        }
        file.ignore(padding);
    }
    return true;
}

bool Image::writeFile(const std::string& path) {
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        std::cout << "Failed to create the file." << std::endl;
        return false;
    }

    file.write(reinterpret_cast<char*>(&file_header), sizeof(file_header));
    file.write(reinterpret_cast<char*>(&info_header), sizeof(info_header));

    int width = info_header.width;
    int height = std::abs(info_header.height);
    int padding = (4 - ((width*3) % 4)) % 4;

    file.seekp(file_header.offset, std::ios::beg);

    for (int i = height - 1; i >= 0; --i) {
        for(int j = 0; j < width; ++j) {
            file.write(reinterpret_cast<char*>(&pixels[i][j]), 3);
        }
        file.write("\0\0\0", padding);
    }
    return true;
}

void Image::clearMemory() {
    for (int i = 0; i < std::abs(info_header.height); ++i) {
        delete[] pixels[i];
    }
    delete[] pixels;
    pixels = nullptr;
}

void Image::rotateClockwise() {
    Pixel **rotated = new Pixel*[info_header.width];
    for (int i = 0; i < info_header.width; ++i) {
        rotated[i] = new Pixel[std::abs(info_header.height)];
    }

    for (int i = 0; i < std::abs(info_header.height); ++i) {
        for (int j = 0; j < info_header.width; ++j) {
            rotated[j][std::abs(info_header.height)-i-1] = pixels[i][j];
        }
    }

    clearMemory();
    pixels = rotated;
    std::swap(info_header.width, info_header.height);
    rotated = nullptr;
}

void Image::rotateCounterClockwise() {
    Pixel **rotated = new Pixel*[info_header.width];
    for (int i = 0; i < info_header.width; ++i) {
        rotated[i] = new Pixel[std::abs(info_header.height)];
    }

    for (int i = 0; i < std::abs(info_header.height); ++i) {
        for (int j = 0; j < info_header.width; ++j) {
            rotated[info_header.width-j-1][i] = pixels[i][j];
        }
    }

    clearMemory();
    pixels = rotated;
    std::swap(info_header.width, info_header.height);
    rotated = nullptr;
}

void Image::gaussianBlur() {
    int width = info_header.width;
    int height = std::abs(info_header.height);

    const int kSize = 3;
    const float kernel[kSize][kSize] = {
        {1 / 16.0f, 2 / 16.0f, 1 / 16.0f},
        {2 / 16.0f, 4 / 16.0f, 2 / 16.0f},
        {1 / 16.0f, 2 / 16.0f, 1 / 16.0f}
    };

    Pixel **temporary = new Pixel*[height];
    for (int i = 0; i < height; ++i) {
        temporary[i] = new Pixel[width];
    }

    for (int y = 1; y < height - 1; ++y) {
        for (int x = 1; x < width - 1; ++x) {
            float red = 0.0f, green = 0.0f, blue = 0.0f;
            for (int ky = -1; ky <= 1; ++ky) {
                for (int kx = -1; kx <= 1; ++kx) {
                    Pixel &p = pixels[y + ky][x + kx];
                    red += p.red * kernel[ky + 1][kx + 1];
                    green += p.green * kernel[ky + 1][kx + 1];
                    blue += p.blue * kernel[ky + 1][kx + 1];
                }
            }
            temporary[y][x].red = static_cast<uint8_t>(red);
            temporary[y][x].green = static_cast<uint8_t>(green);
            temporary[y][x].blue = static_cast<uint8_t>(blue);
        }
    }

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (y < 1 || y >= height - 1 || x < 1 || x >= width - 1) {
                temporary[y][x] = pixels[y][x];
            }
        }
    }

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            pixels[y][x] = temporary[y][x];
        }
        delete[] temporary[y];
    }
    delete[] temporary;
}
