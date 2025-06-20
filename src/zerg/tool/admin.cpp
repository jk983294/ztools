
#include <fcntl.h>
#include <pthread.h>
#include <pwd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstdlib>
#include <iostream>
#include <vector>
#include <zerg/time/time.h>
#include <zerg/log.h>
#include <zerg/unix.h>
#include <zerg/tool/admin.h>

using namespace std;

namespace zerg {
Admin::Admin(const std::string& shm_key_) : shm_key{shm_key_} {
    if (shm_key.empty()) {
        ZLOG_THROW("admin key cannot be empty");
    }
    shm_name += shm_key;
}

Admin& Admin::OpenForCreate() {
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

Admin& Admin::OpenForRead() {
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
    ZLOG("AdminShm %s opened, size %zu", shm_name.c_str(), shm_length);
    return *this;
}

void Admin::IssueCmd(const string& cmd) const {
    if (!pAdminShm) {
        ZLOG("pAdminShm not ready");
        return;
    }
    if (cmd.empty() || cmd.size() > sizeof(pAdminShm->cmd)) {
        ZLOG("cmd length not correct");
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
    ZLOG("issue cmd '%.128s' from %.16s at %s success", pAdminShm->cmd, pAdminShm->issuer, time_str.c_str());
}

string Admin::ReadCmd() {
    if (pAdminShm && pAdminShm->cmd_update_time > last_cmd_time) {
        // printf("recv cmd '%s' from %s at %ld\n", pAdminShm->cmd, pAdminShm->issuer, pAdminShm->update_time);
        last_cmd_time = pAdminShm->cmd_update_time;
        return string{pAdminShm->cmd};
    } else {
        return "";
    }
}

void Admin::WriteReturn(const string& ret) {
    if (!pAdminShm) {
        ZLOG("pAdminShm not ready");
        return;
    }
    if (ret.empty() || ret.size() > sizeof(pAdminShm->ret)) {
        ZLOG("ret length not correct");
        return;
    }
    strcpy(pAdminShm->ret, ret.c_str());
    pAdminShm->ret_update_time = zerg::GetMicrosecondsSinceEpoch();
    auto time_str = zerg::time_t2string(pAdminShm->ret_update_time / 1000000);
    ZLOG("write return '%.128s' from %.16s at %s success", pAdminShm->ret, pAdminShm->issuer, time_str.c_str());
}

string Admin::ReadReturn() {
    if (pAdminShm && pAdminShm->ret_update_time > pAdminShm->cmd_update_time) {
        return string{pAdminShm->ret};
    } else if (pAdminShm && !CheckProcessAlive(pAdminShm->consumerPID)) {
        return "DIE";
    } else {
        return "";
    }
}
}
