#pragma once

#include <fcntl.h>
#include <pthread.h>
#include <pwd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>
#include <zerg/time/time.h>
#include <zerg/log.h>
#include <zerg/unix.h>

using namespace std;

namespace zerg {
struct __attribute__((packed)) AdminShmData {
    char cmd[1024] = {};
    char ret[1024] = {};
    char issuer[128] = {};
    pid_t consumerPID = 0;
    long create_time = 0;      // microseconds since UNIX EPOCH
    long cmd_update_time = 0;  // microseconds since UNIX EPOCH
    long ret_update_time = 0;
};

struct Admin {
    bool shm_status{false};
    std::string shm_key;
    string shm_name{"admin_"};
    AdminShmData* pAdminShm{nullptr};
    long last_cmd_time{0};

    Admin(const std::string& shm_key_) : shm_key{shm_key_} {
        if (shm_key.empty()) {
            ZLOG_THROW("admin key cannot be empty");
        }
        shm_name += shm_key;
    }

    Admin& OpenForCreate() {
        auto fd = shm_open(shm_name.c_str(), O_CREAT | O_RDWR, 0666);
        if (fd == -1) {
            ZLOG_THROW("cannot create shm for %s", shm_name.c_str());
        }
        fchmod(fd, 0777);
        auto ret = ftruncate(fd, sizeof(AdminShmData));
        if (ret != 0) {
            ZLOG_THROW("failed to truncate shm %s", shm_name.c_str());
        }
        char* p_mem = (char*)mmap(nullptr, sizeof(AdminShmData), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (p_mem == MAP_FAILED) {
            ZLOG_THROW("failed to mmap shm %s", shm_name.c_str());
        }
        printf("AdminShm %s created with size:%zu\n", shm_name.c_str(), sizeof(AdminShmData));
        pAdminShm = reinterpret_cast<AdminShmData*>(p_mem);
        memset(p_mem, 0, sizeof(AdminShmData));
        pid_t uid = getpid();
        if (uid) {
            pAdminShm->consumerPID = uid;
        }
        return *this;
    }

    Admin& OpenForRead() {
        auto shm_length = sizeof(AdminShmData);
        auto fd = shm_open(shm_name.c_str(), O_RDWR, 0666);
        if (fd == -1) {
            shm_status = false;
            perror("shm open failed\n");
            close(fd);
            return *this;
        }
        char* p_mem = (char*)mmap(nullptr, shm_length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (p_mem == MAP_FAILED) {
            shm_status = false;
            perror("MEM MAP FAILED\n");
            return *this;
        }

        pAdminShm = reinterpret_cast<AdminShmData*>(p_mem);
        shm_status = true;
        printf("AdminShm %s opened, size %zu\n", shm_name.c_str(), shm_length);
        return *this;
    }

    void IssueCmd(const string& cmd) const {
        if (!pAdminShm) {
            printf("pAdminShm not ready\n");
            return;
        }
        if (cmd.empty() || cmd.size() > sizeof(pAdminShm->cmd)) {
            printf("cmd length not correct\n");
            return;
        }
        strcpy(pAdminShm->cmd, cmd.c_str());
        uid_t uid = geteuid();
        struct passwd* pw = getpwuid(uid);
        if (pw) {
            strcpy(pAdminShm->issuer, pw->pw_name);
        }
        pAdminShm->cmd_update_time = zerg::GetMicrosecondsSinceEpoch();
        auto time_str = zerg::time_t2string(pAdminShm->cmd_update_time / 1000000);
        printf("issue cmd '%s' from %s at %s success\n", pAdminShm->cmd, pAdminShm->issuer, time_str.c_str());
    }

    string ReadCmd() {
        if (pAdminShm && pAdminShm->cmd_update_time > last_cmd_time) {
            // printf("recv cmd '%s' from %s at %ld\n", pAdminShm->cmd, pAdminShm->issuer, pAdminShm->update_time);
            last_cmd_time = pAdminShm->cmd_update_time;
            return string{pAdminShm->cmd};
        } else {
            return "";
        }
    }

    void WriteReturn(const string& ret) {
        if (!pAdminShm) {
            printf("pAdminShm not ready\n");
            return;
        }
        if (ret.empty() || ret.size() > sizeof(pAdminShm->ret)) {
            printf("ret length not correct\n");
            return;
        }
        strcpy(pAdminShm->ret, ret.c_str());
        pAdminShm->ret_update_time = zerg::GetMicrosecondsSinceEpoch();
        auto time_str = zerg::time_t2string(pAdminShm->ret_update_time / 1000000);
        printf("write return '%s' from %s at %s success\n", pAdminShm->ret, pAdminShm->issuer, time_str.c_str());
    }

    string ReadReturn() {
        if (pAdminShm && pAdminShm->ret_update_time > pAdminShm->cmd_update_time) {
            return string{pAdminShm->ret};
        } else if (pAdminShm && !CheckProcessAlive(pAdminShm->consumerPID)) {
            return "DIE";
        } else {
            return "";
        }
    }
};
}
