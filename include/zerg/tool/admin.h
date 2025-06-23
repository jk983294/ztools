#pragma once

#include <pthread.h>
#include <string>
#include <zerg/io/shm.h>

using namespace std;

namespace zerg {
struct __attribute__((packed)) AdminShmData {
    ShmHeader header;
    char cmd[1024] = {};
    char ret[1024] = {};
    char issuer[128] = {};
    pid_t consumerPID = 0;
    long cmd_update_time = 0;  // microseconds since UNIX EPOCH
    long ret_update_time = 0;
};

struct Admin {
    string shm_name;
    AdminShmData* pAdminShm{nullptr};
    long last_cmd_time{0};

    explicit Admin(const std::string& shm_key_);

    bool OpenForCreate();

    bool OpenForRead();

    void IssueCmd(const string& cmd) const;

    string ReadCmd();

    void WriteReturn(const string& ret);

    string ReadReturn();
};
}
