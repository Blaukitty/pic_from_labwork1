/* Kniazeva Eva Andreevna
st128104@student.spbu.ru 
labwork 1 */

#include "Image.h"
#include <iostream>

int main() {
    Image picture;
    if (!picture.readFile("input.bmp")) {
        std::cout << "Could not read the input file." << std::endl;
        return 1;
    }

    picture.rotateClockwise();
    if (!picture.writeFile("clockwise.bmp")) {
        std::cout << "Could not save the 1st output." << std::endl;
        return 1;
    }

    picture.readFile("input.bmp");
    picture.rotateCounterClockwise();
    if (!picture.writeFile("counterclockwise.bmp")) {
        std::cout << "Could not save the 2nd output." << std::endl;
        return 1;
    }

    picture.readFile("input.bmp");
    picture.gaussianBlur();
    if (!picture.writeFile("gaussian.bmp")) {
        std::cout << "Could not save the 3rd output." << std::endl;
        return 1;
    }
}
