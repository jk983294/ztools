
#include <fcntl.h>
#include <pwd.h>
#include <unistd.h>
#include <zerg/log.h>
#include <zerg/time/time.h>
#include <zerg/tool/admin.h>
#include <zerg/unix.h>
#include <iostream>
#include <zerg/time/clock.h>

using namespace std;

namespace zerg {
Admin::Admin(const std::string& shm_key_) {
    if (shm_key_.empty()) {
        ZLOG_THROW("admin key cannot be empty");
    }
    shm_name = shm_key_;
}

bool Admin::OpenForCreate() {
    Clock clock(true);
    char* p_mem = CreateShm(shm_name, sizeof(AdminShmData), 1042, clock.DateToInt(), true);
    pAdminShm = reinterpret_cast<AdminShmData*>(p_mem);
    pid_t uid = getpid();
    if (uid) {
        pAdminShm->consumerPID = uid;
    }
    return pAdminShm != nullptr;
}

bool Admin::OpenForRead() {
    char* p_mem = LinkShm(shm_name, 1042, true);
    pAdminShm = reinterpret_cast<AdminShmData*>(p_mem);
    ZLOG("AdminShm %s opened, size %zu", shm_name.c_str(), pAdminShm->header.size);
    return pAdminShm != nullptr;
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
