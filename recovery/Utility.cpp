//
// Created by Frank Steiler on 4/13/17.
// Copyright (c) 2017 Hewlett-Packard. All rights reserved.
//
// Utility.cpp: [...]
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