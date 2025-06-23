#pragma once

#include <string>

namespace zerg {
struct __attribute__((packed)) ShmHeader {
    int32_t magic;
    int32_t date;
    uint64_t size;
    int64_t creation_time; // nanoSinceEpoch
};

/**
 * Open file-backed share memory with file path and name path_file_name.
 * open only if it's already exists, try to create new one if not.
 */
char* CreateShm(std::string& path, uint64_t size, int32_t magic, int32_t date, bool lock= false, bool reset = true);
char* LinkShm(std::string& path, int32_t magic);
}
