#pragma once

#include <pthread.h>
#include <string>

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

    Admin(const std::string& shm_key_);

    Admin& OpenForCreate();

    Admin& OpenForRead();

    void IssueCmd(const string& cmd) const;

    string ReadCmd();

    void WriteReturn(const string& ret);

    string ReadReturn();
};
}
