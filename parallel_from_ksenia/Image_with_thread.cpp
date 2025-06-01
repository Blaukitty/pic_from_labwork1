#include "Image_K.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <thread>
#include <algorithm>

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

static void rotateClockwiseWorker(  //копирует строки перевернутой на 90 градусов картинки
    Pixel** pixels,
    Pixel** rotated,
    int oldW,
    int oldH,
    int i_start,
    int i_end
) {
    for (int i = i_start; i < i_end; ++i) {
        for (int j = 0; j < oldW; ++j) {
            rotated[j][oldH - i - 1] = pixels[i][j];
        }
    }
}

// отсюда начинаем разбивать на потоки, до этого код евы без изменений
void Image::rotateClockwise() {  // по часовой стрелке
    int oldW = info_header.width;
    int oldH = std::abs(info_header.height);

    // новый буфер изо
    Pixel **rotated = new Pixel*[info_header.width];
    for (int i = 0; i < oldW; ++i) {
        rotated[i] = new Pixel[oldH];
    }

    // определяем число потоков
    unsigned int numThr = std::thread::hardware_concurrency();
    if (numThr == 0) {
        numThr = 2;
    }

    int chunk = (OldW) / numThreads;
    int rem = (OldH) % numThreads;

    std::vector<std::thread> threads;
    threads.reserve(numThr);

    int cur = 0;
    for (unsigned int t = 0; t < numThreads; ++t) {
        int start = cur;
        int end   = cur + chunk + (t < rem ? 1 : 0); //Запуск потока, который внутри вызовет rotateClockwiseWorker
        threads.emplace_back(
            rotateClockwiseWorker,
            pixels,      
            rotated,     
            oldW,        
            oldH,        
            start,       
            end          
        );
        cur = end;
    }

    for (auto &t : threads) {  // ожидаем конца всех потоков
        t.join();
    }
    
    clearMemory();
    pixels = rotated;
    std::swap(info_header.width, info_header.height);
    rotated = nullptr;
}

static void rotateCounterClockwiseWorker( 
    Pixel** pixels,
    Pixel** rotated,
    int oldW,
    int oldH,
    int i_start,
    int i_end
) {
    for (int i = i_start; i < i_end; ++i) {
        for (int j = 0; j < oldW; ++j) {
            // Формула поворота против часовой стрелки
            rotated[oldW - j - 1][i] = pixels[i][j];
        }
    }
}

void Image::rotateCounterClockwise() { // против часовой стрелки, все аналогично
    int oldW = info_header.width;
    int oldH = std::abs(info_header.height);
    
    Pixel **rotated = new Pixel*[info_header.width];
    for (int i = 0; i < OldW; ++i) {
        rotated[i] = new Pixel[OldH];
    }

    unsigned int numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 2;

    int chunk = oldH / numThr;
    int rem   = oldH % numThr;

    std::vector<std::thread> threads;
    threads.reserve(numThreads);

    int cur = 0;
    for (unsigned int t = 0; t < numThreads; ++t) {
        int start = cur;
        int end   = cur + chunk + (t < rem ? 1 : 0);
        threads.emplace_back(
            rotateCounterClockwiseWorker,
            pixels,
            rotated,
            oldW,
            oldH,
            start,
            end
        );
        cur = end;
    }

    for (auto &th : threads) {
        th.join();
    }

    clearMemory();
    pixels = rotated;
    std::swap(info_header.width, info_header.height);
    rotated = nullptr;
}

static void gaussianBlurWorker(
    Pixel** pixels,
    Pixel** temporary,
    int width,
    int height,
    int y_start,
    int y_end
) {
    const int kSize = 3;
    const float kernel[kSize][kSize] = {
        {1/16.f, 2/16.f, 1/16.f},
        {2/16.f, 4/16.f, 2/16.f},
        {1/16.f, 2/16.f, 1/16.f}
    };

    for (int y = y_start; y < y_end; ++y) {
        // Пропускаем граничные строки — они копируются отдельно
        if (y < 1 || y >= height - 1) continue;

        for (int x = 1; x < width - 1; ++x) {
            float red = 0.f, green = 0.f, blue = 0.f;
            for (int ky = -1; ky <= 1; ++ky) {
                for (int kx = -1; kx <= 1; ++kx) {
                    Pixel &p = pixels[y + ky][x + kx];
                    red   += p.red   * kernel[ky + 1][kx + 1];
                    green += p.green * kernel[ky + 1][kx + 1];
                    blue  += p.blue  * kernel[ky + 1][kx + 1];
                }
            }
            temporary[y][x].red   = static_cast<uint8_t>(red);
            temporary[y][x].green = static_cast<uint8_t>(green);
            temporary[y][x].blue  = static_cast<uint8_t>(blue);
        }
    }
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

    Pixel **temporary = new Pixel*[height]; // временный буфер
    for (int i = 0; i < height; ++i) {
        temporary[i] = new Pixel[width];
    }

    unsigned int numThr = std::thread::hardware_concurrency();
    if (numThr == 0) numThr = 2;

    int chunk = height / numThr;
    int rem   = height % numThr;

    std::vector<std::thread> threads;
    threads.reserve(numThr);

   int cur = 0;
    for (unsigned int t = 0; t < numThr; ++t) {
        int start = cur;
        int end   = cur + chunk + (t < rem ? 1 : 0);
        threads.emplace_back(
            gaussianBlurWorker,
            pixels,       // исходный массив
            temporary,    // буфер для результатов
            width,
            height,
            start,
            end
        );
        cur = end;
    }

    // 4) Ждём завершения всех «воркеров»
    for (auto &th : threads) {
        th.join();
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
