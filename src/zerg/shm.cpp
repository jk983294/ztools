#include <zerg/io/shm.h>
#include <zerg/log.h>
#include <zerg/time/time.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

namespace zerg {
char* CreateShm(const std::string& shm_name, uint64_t size, int32_t magic, int32_t date, bool lock, bool reset) {
    if (size < sizeof(ShmHeader)) {
        ZLOG_THROW("shm size too small %zu", size);
    }
    char* p_mem = nullptr;
    int fd = open(shm_name.c_str(), O_RDWR, 0666);
    if (fd != -1) {
        ZLOG("Shm %s already created, linking......", shm_name.c_str());
        p_mem = (char*)mmap64(nullptr, sizeof(ShmHeader), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (!p_mem) {
            ZLOG_THROW("Cannot mmap header due to %s", strerror(errno));
        }
        ShmHeader header = *(ShmHeader*)p_mem;
        if (header.size != size) {
            ZLOG_THROW("shm header size not match! %zu <> %zu", header.size, size);
        }
        if (header.magic != magic) {
            ZLOG_THROW("shm header magic not match! %d <> %d", header.magic, magic);
        }
        munmap(p_mem, sizeof(ShmHeader));
    } else { // Create new one
        fd = open(shm_name.c_str(), O_CREAT | O_RDWR, 0666);
        if (fd == -1) {
            ZLOG_THROW("cannot create shm %s", shm_name.c_str());
        }
        fchmod(fd, 0777);
        auto ret = ftruncate(fd, size);
        if (ret != 0) {
            ZLOG_THROW("failed to truncate shm %s due to %s", shm_name.c_str(), strerror(errno));
        }
    }
    p_mem = (char*)mmap64(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (p_mem == MAP_FAILED) {
        ZLOG_THROW("failed to mmap shm %s due to %s", shm_name.c_str(), strerror(errno));
    }
    if (lock) {
        if (mlock(p_mem, size)) {
            ZLOG("warn! %s", strerror(errno));
        }
    }
    if (reset) {
        memset(p_mem, 0, size);
    }
    auto* pHeader = reinterpret_cast<ShmHeader*>(p_mem);
    pHeader->magic = magic;
    pHeader->date = date;
    pHeader->size = size;
    int32_t pid = getpid();
    memcpy(&pHeader->pid, &pid, sizeof(int32_t));
    pHeader->creation_time = nanoSinceEpoch();
    ZLOG("shm=%s created date=%d, size=%zu, creation_time=%ld", shm_name.c_str(), date, size, pHeader->creation_time);
    return p_mem;
}

char* LinkShm(const std::string& shm_name, int32_t magic) {
    auto fd = open(shm_name.c_str(), O_RDWR, 0666);
    if (fd == -1) {
        ZLOG_THROW("Cannot shm_open file %s due to %s", shm_name.c_str(), strerror(errno));
    }
    char* p_mem = (char*)mmap64(nullptr, sizeof(ShmHeader), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (!p_mem) {
        ZLOG_THROW("Cannot mmap file %s", shm_name.c_str());
    }
    ShmHeader header = *reinterpret_cast<ShmHeader*>(p_mem);
    if (header.magic != magic) {
        ZLOG_THROW("mmap header magic mismatch %d <> %d", header.magic, magic);
    }
    munmap(p_mem, sizeof(ShmHeader));

    p_mem = (char*)mmap64(nullptr, header.size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (p_mem == MAP_FAILED) {
        perror("MEM MAP FAILED\n");
        return nullptr;
    }
    ZLOG("link to shm=%s created date=%d, size=%zu, creation_time=%ld", shm_name.c_str(), header.date, header.size, header.creation_time);
    return p_mem;
}

void ReleaseShm(char* data, uint64_t size) {
    if (data == nullptr) return;
    if (size == 0) {
        size = reinterpret_cast<ShmHeader*>(data)->size;
    }
    munmap(data, size);
}
}
