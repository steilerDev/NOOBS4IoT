//
// Created by Frank Steiler on 4/13/17 as part of NOOBS4IoT (https://github.com/steilerDev/NOOBS4IoT)
//
// Utility.cpp:
//      This file contains general utility functions required by the application.
//      For more information see https://github.com/steilerDev/NOOBS4IoT/wiki.
//
// This file is licensed under a GNU General Public License v3.0 (c) Frank Steiler.
// See https://raw.githubusercontent.com/steilerDev/NOOBS4IoT/master/LICENSE for more information.
//

#include "Utility.h"

// Prints the given QR Code to the console.
void Utility::printQrCode(const qrcodegen::QrCode &qrCode) {
    int border = 4;
    for (int y = -border; y < qrCode.size + border; y++) {
        for (int x = -border; x < qrCode.size + border; x++) {
            std::cout << (qrCode.getModule(x, y) == 1 ? "\u2588\u2588" : "  ");
        }
        std::cout << std::endl;
    }
}