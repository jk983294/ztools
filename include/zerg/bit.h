#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

namespace zerg {

template <typename T>
void PrintHex(T val) {
    const size_t sz = sizeof(T);
    uint8_t* newP = (uint8_t*)&val;
    for (size_t i = 0; i < sz; ++i) {
        printf("%02X ", newP[i]);
    }
}

template <typename T>
T BitSwap(T val) {
    T retVal;
    int8_t* pVal = (int8_t*)&val;
    int8_t* pRetVal = (int8_t*)&retVal;
    int size = sizeof(T);
    for (int i = 0; i < size; ++i) pRetVal[size - 1 - i] = pVal[i];
    return retVal;
}

}
