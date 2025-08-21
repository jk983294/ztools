#pragma once

#include <dlfcn.h>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

namespace zerg {
/**
 * c++ version of mkdir -p command
 * example: mkdirp("/tmp/dd/aaa", 0755);
 * @param s path
 * @param mode new directory mode
 * @return flags
 */
int mkdirp(std::string s, mode_t mode = 0755);

/**
 * check if file exist
 * @param name ull path of file
 * @return
 */
bool IsFileExisted(const std::string& name);

int ChangeFileMode(const std::string& path, mode_t mode);

int LockFile(const std::string& locker);
void UnlockFile(int fd);

/**
 * join paths on linux
 * path_join("/tmp", "aa", "", "bb/", "cc"); // => /tmp/aa/bb/cc
 */
template <class... Args>
std::string path_join(const std::string& path, const Args... args) {
    std::array<std::string, sizeof...(args)> name_list = {{args...}};
    std::string ret = path;
    for (size_t i = 0; i < name_list.size(); ++i) {
        if (!ret.empty())
            if (*ret.rbegin() != '/') ret += "/";
        if (name_list[i].empty()) continue;
        ret += name_list[i];
    }
    return ret;
}

std::vector<std::string> ListDir(const std::string& directory);

std::string FileExpandUser(const std::string& name);

std::string GetAbsolutePath(const std::string& rawPath);

bool IsFileReadable(const std::string& filename);

bool IsFileWritable(const std::string& filename);

// check if the file descriptor is unlinked
int IsFdDeleted(int fd);

std::string Dirname(const std::string& fullname);

std::string Basename(const std::string& fullname);

bool IsDirReadable(const std::string& dirname);

bool IsDirWritable(const std::string& dirname);

bool IsFile(const std::string& pathname);

time_t GetLastModificationTime(const std::string& pathname);

off_t GetFileSize(const std::string& pathname);

bool IsDir(const std::string& pathname);
bool EnsureDir(const std::string& dir);

std::string GetLastLineOfFile(const std::string& pathname);

struct SoInfo {
    void* handler;
    int major;
    int minor;
    int patch;
    std::string build;
    std::string compiler;
    std::string compiler_version;
    std::string so_path;
};

SoInfo loadSO(const std::string& so_path, char type = RTLD_LAZY);
void* internal_create_from_so(const std::string& so_path, const std::string& name);

bool read_trading_days(const std::string& day_file_path, std::vector<int>& days);

// like /tmp/*.csv
std::unordered_map<std::string, std::string> path_wildcard(const std::string& path);

/**
 *
 * @param dir path to traverse
 * @param func the callback function to process each file
 * @param is_enter_sub
 * @param need_folder
 */
void dir_traversal(const std::string& dir, void (*func)(const std::string&), bool is_enter_sub = false,
                          bool need_folder = false);

}
