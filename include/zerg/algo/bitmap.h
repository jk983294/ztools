#pragma once

#include <iostream>
#include "zerg_intlist.h"

namespace zerg {

class zerg_bitmap {
public:
    size_t x_{0}, y_{0};        // size of row, size of col
    uint8_t* matrix_{nullptr};  // memory area to store bitmap
public:
    zerg_bitmap(size_t sizeX, size_t sizeY) {
        matrix_ = new unsigned char[(sizeY / 8 + 1) * sizeX];
        x_ = sizeX;
        y_ = sizeY;
        Zero();
    }

    zerg_bitmap() = default;
    ~zerg_bitmap() { delete[] matrix_; }

    // set specific bit
    void Set(const size_t& x, const size_t& y, int val) {
        if (val == 0)
            matrix_[x * (y_ / 8 + 1) + (y / 8)] = matrix_[x * (y_ / 8 + 1) + (y / 8)] & (~(1 << (y % 8)));
        else
            matrix_[x * (y_ / 8 + 1) + (y / 8)] = matrix_[x * (y_ / 8 + 1) + (y / 8)] | (1 << (y % 8));
    }

    // get specific bit
    int Get(const size_t& x, const size_t& y) const {
        return ((matrix_[x * (y_ / 8 + 1) + (y / 8)] & (1 << (y % 8))) >> (y % 8));
    }

    void Size(size_t& nox, size_t& noy) const {
        nox = x_;
        noy = y_;
    }

    // zero out all bits
    void Zero() {
        size_t c;
        for (c = 0; c < (y_ / 8 + 1) * x_; c++) matrix_[c] = 0;
    }

    // ones all bits
    void One() {
        size_t c;
        for (c = 0; c < (y_ / 8 + 1) * x_; c++) matrix_[c] = 1;
    }

    // print to screen
    void Show() const {
        size_t x, y;
        for (x = 0; x < x_; x++) {
            for (y = 0; y < y_; y++) {
                std::cout << __bitmap_print(static_cast<uint32_t>(Get(x, y))) << ' ';
            }
            std::cout << '\n';
        }
    }
    // Count total bits
    unsigned long Count() const {
        size_t x, y;
        unsigned long int c = 0;
        for (y = 0; y < y_; y++) {
            for (x = 0; x < x_; x++) {
                if (Get(x, y) == 1) c++;
            }
        }
        return c;
    }

    // Count specific column
    unsigned long CountCol(const size_t col) const {
        size_t x;
        unsigned long int c = 0;
        for (x = 0; x < x_; ++x) {
            if (Get(x, col) == 1) c++;
        }
        return c;
    }

    // Count specific row
    unsigned long CountRow(const size_t row) const {
        size_t y;
        unsigned long int c = 0;
        for (y = 0; y < y_; ++y) {
            if (Get(row, y) == 1) c++;
        }
        return c;
    }

    char __bitmap_print(uint32_t x) const { return x == 0 ? '-' : '*'; }
};

}
