#pragma once

#include <malloc.h>
#include <cstdint>

namespace zerg {

class IntList {
public:
    IntList() = default;

    ~IntList() {
        if (data) free(data);
    }

    size_t size() const { return count; }

    // Note: no check for out-of-bounds
    uint32_t& operator[](int index) const { return data[index]; }

    void add(uint32_t i) {
        if (count >= capacity) {
            size_t new_size = count + 8;
            resize(new_size);
        }
        data[count++] = i;
    }

    void clear() {
        if (data) free(data);
        data = nullptr;
        capacity = 0;
        count = 0;
    }

private:
    size_t capacity{0}, count{0};
    uint32_t* data{nullptr};

    void resize(size_t new_size) {
        if (new_size == 0) {
            clear();
            return;
        }
        data = (uint32_t*)realloc(data, new_size * sizeof(*data));
        capacity = new_size;
    }
};

}
