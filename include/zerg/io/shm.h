#pragma once

#include <string>
#include <cstdint>

namespace zerg {
struct __attribute__((packed)) ShmHeader {
    int32_t magic;
    int32_t date;
    int32_t pid;
    int32_t reserve;
    uint64_t size; // total size
    int64_t creation_time; // nanoSinceEpoch
};

/**
 * Open file-backed share memory with file path and name path_file_name.
 * open only if it's already exists, try to create new one if not.
 */
char* CreateShm(const std::string& path, uint64_t size, int32_t magic, int32_t date, bool lock= false, bool reset = true);
char* LinkShm(const std::string& path, int32_t magic);
/**
 * size = 0, use ShmHeader.size to release
 */
void ReleaseShm(char* data, uint64_t size = 0);
}
