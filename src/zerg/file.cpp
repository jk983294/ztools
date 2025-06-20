#include <dirent.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <zerg/io/file.h>
#include <zerg/log.h>
#include <zerg/string.h>
#include <algorithm>

namespace zerg {
int mkdirp(std::string s, mode_t mode) {
    size_t pre = 0, pos;
    int ret = -1;
    
    if (s[s.size() - 1] != '/') {
        s = s + std::string("/");  // add trailing / if not found
    }
    while ((pos = s.find_first_of('/', pre)) != std::string::npos) {
        std::string dir = s.substr(0, pos++);
        pre = pos;
        if (dir.empty()) continue;  // if leading / first time is 0 length
        if ((ret = mkdir(dir.c_str(), mode)) && errno != EEXIST) {
            return ret;
        }
    }

    if (ret && errno != EEXIST)
        return ret;
    else
        return 0;
}

/**
 * check if file exist
 * @param name ull path of file
 * @return
 */
bool IsFileExisted(const std::string& name) {
    struct stat buffer;
    return (stat(name.c_str(), &buffer) == 0);
}

int ChangeFileMode(const std::string& path, mode_t mode) { return chmod(path.c_str(), mode); }

int LockFile(const std::string& locker) {
    if (!IsFileExisted(locker)) {
        std::ofstream of(locker);
        of.close();
    }
    ChangeFileMode(locker, 0777);
    const char* file = locker.c_str();
    int fd;
    struct flock lock;
    // Open a file descriptor to the file
    fd = open(file, O_WRONLY);
    // Initialize the flock structure
    memset(&lock, 0, sizeof(lock));
    lock.l_type = F_WRLCK;
    // Place a write lock on the file
    fcntl(fd, F_SETLKW, &lock);
    return fd;
}

void UnlockFile(int fd) {
    struct flock lock;
    memset(&lock, 0, sizeof(lock));
    // Release the lock
    lock.l_type = F_UNLCK;
    fcntl(fd, F_SETLKW, &lock);
    close(fd);
}

std::vector<std::string> ListDir(const std::string& directory) {
    auto dir = opendir(directory.c_str());
    std::vector<std::string> ret;
    if (nullptr == dir) {
        return ret;
    }

    auto entity = readdir(dir);
    while (entity != nullptr) {
        if (entity->d_name[0] == '.' &&
            (entity->d_name[1] == '\0' || (entity->d_name[1] == '.' && entity->d_name[2] == '\0'))) {
            entity = readdir(dir);
            continue;
        }
        auto node = path_join(directory, entity->d_name);
        ret.push_back(node);
        entity = readdir(dir);
    }
    closedir(dir);
    return ret;
}

std::string FileExpandUser(const std::string& name) {
    if (name.empty() || name[0] != '~') return name;
    char user[128];
    getlogin_r(user, 127);
    return std::string{"/home/"} + user + name.substr(1);
}

std::string GetAbsolutePath(const std::string& rawPath) {
    if (rawPath.empty() || rawPath == ".") {
        char* pwd = getcwd(nullptr, 0);
        std::string ret = pwd;
        free(pwd);
        return ret;
    } else if (rawPath[0] == '~') {
        return FileExpandUser(rawPath);
    } else if (rawPath[0] != '/') {
        char* pwd = getcwd(nullptr, 0);
        std::string cur = pwd;
        free(pwd);
        if (rawPath.find("./") == 0)
            return cur + rawPath.substr(1);
        else
            return cur + '/' + rawPath;
    }
    return rawPath;
}

bool IsFileReadable(const std::string& filename) { return (access(filename.c_str(), R_OK) != -1); }

bool IsFileWritable(const std::string& filename) { return (access(filename.c_str(), W_OK) != -1); }

// check if the file descriptor is unlinked
int IsFdDeleted(int fd) {
    struct stat _stat;
    int ret = -1;
    auto code = fcntl(fd, F_GETFL);
    if (code != -1) {
        if (!fstat(fd, &_stat)) {
            std::cout << _stat.st_nlink << std::endl;
            if (_stat.st_nlink >= 1)
                ret = 0;
            else
                ret = -1;  // deleted
        }
    }
    if (errno != 0) perror("IsFdDeleted");
    return ret;
}

std::string Dirname(const std::string& fullname) {
    char p[256];
    strncpy(p, fullname.c_str(), sizeof(p) - 1);
    return std::string{dirname(p)};
}

std::string Basename(const std::string& fullname) {
    char p[256];
    strncpy(p, fullname.c_str(), sizeof(p) - 1);
    return std::string{basename(p)};
}

bool IsDirReadable(const std::string& dirname) { return (access(dirname.c_str(), R_OK) != -1); }

bool IsDirWritable(const std::string& dirname) { return (access(dirname.c_str(), W_OK) != -1); }

bool IsFile(const std::string& pathname) {
    struct stat st{};
    if (lstat(pathname.c_str(), &st) == -1) return false;
    if (S_ISDIR(st.st_mode))
        return false;
    else
        return true;
}

time_t GetLastModificationTime(const std::string& pathname) {
    struct stat st{};
    if (lstat(pathname.c_str(), &st) == -1) return 0;
    return st.st_mtime;
}

off_t GetFileSize(const std::string& pathname) {
    struct stat st{};
    if (lstat(pathname.c_str(), &st) == -1) return -1;
    return st.st_size;
}

bool IsDir(const std::string& pathname) {
    struct stat st{};
    if (lstat(pathname.c_str(), &st) == -1) return false;
    if (S_ISDIR(st.st_mode))
        return true;
    else
        return false;
}

std::string GetLastLineOfFile(const std::string& pathname) {
    std::ifstream read(pathname, std::ios_base::ate);  // open file
    std::string tmp;

    if (read) {
        long length = read.tellg();  // get file size

        // loop backward over the file
        for (long i = length - 2; i > 0; i--) {
            read.seekg(i);
            char c = read.get();
            if (c == '\r' || c == '\n') break;
        }

        std::getline(read, tmp);  // read last line
    }
    return tmp;
}

SoInfo loadSO(const std::string& so_path, char type) {
    if (!IsFileExisted(so_path)) {
        ZLOG_THROW("cannot found so file in path: %s", so_path.c_str());
    }
    SoInfo info;
    info.so_path = so_path;
    info.handler = dlopen(so_path.c_str(), type);
    if (!info.handler) {
        ZLOG_THROW("cannot found so file in path: %s for the reason: %s", so_path.c_str(), dlerror());
    }
    return info;
}

bool read_trading_days(const std::string& day_file_path, std::vector<int>& days) {
    days.clear();
    auto path = FileExpandUser(day_file_path);
    std::ifstream ifs(path, std::ifstream::in);

    if (ifs.is_open()) {
        string s;
        while (getline(ifs, s)) {
            zerg::Trim(s);
            if (!s.empty() && std::isdigit(s[0])) {
                days.push_back(std::atoi(s.c_str()));
            }
        }
        ifs.close();
    } else {
        return false;
    }
    std::sort(days.begin(), days.end());
    return true;
}

// like /tmp/*.csv
std::unordered_map<std::string, std::string> path_wildcard(const std::string& path) {
    std::unordered_map<std::string, std::string> ret;
    std::string base_name = zerg::Basename(path);
    auto pos = base_name.find('*');
    if (pos != std::string::npos) {
        std::string prefix = base_name.substr(0, pos);
        std::string suffix = base_name.substr(pos + 1);
        std::string dir_path = zerg::Dirname(path) + "/";
        DIR* dir = opendir(dir_path.c_str());
        if (dir) {
            struct dirent* ent;
            while ((ent = readdir(dir)) != nullptr) {
                string file_name = ent->d_name;
                if (zerg::start_with(file_name, prefix) && zerg::end_with(file_name, suffix)) {
                    string matched = file_name.substr(prefix.size(), file_name.size() - prefix.size() - suffix.size());
                    ret.insert({matched, dir_path + file_name});
                }
            }
            closedir(dir);
        }
    }
    return ret;
}

/**
 *
 * @param dir path to traverse
 * @param func the callback function to process each file
 * @param is_enter_sub
 * @param need_folder
 */
void dir_traversal(const std::string& dir, void (*func)(const std::string&), bool is_enter_sub, bool need_folder) {
    std::string full_path;
    if (dir.empty())
        full_path = "";
    else {
        if (dir[dir.size() - 1] == '/')
            full_path = dir;
        else
            full_path = dir + "/";
    }
    DIR* d;
    struct dirent* file;
    struct stat sb {};
    if (!(d = opendir(full_path.c_str()))) return;
    while ((file = readdir(d)) != nullptr) {
        if (file->d_name[0] == '.') continue;
        std::string fname = full_path + file->d_name;
        stat(fname.c_str(), &sb);
        if (S_ISDIR(sb.st_mode)) {
            if (is_enter_sub)
                dir_traversal(fname, func, is_enter_sub);
            else if (need_folder)
                func(std::string("\x01") + fname);
        } else
            func(fname);
    }
    closedir(d);
}
}