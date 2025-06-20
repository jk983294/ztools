#include <unistd.h>
#include <fstream>
#include <zerg/io/IniReader.h>
#include <zerg/io/file.h>
#include <zerg/unix.h>
#include <zerg/proc.h>
#include <zerg/log.h>

namespace zerg {

int WriteProgramPid(const std::string& program) {
    pid_t pid = getpid();
    std::ofstream ofs;
    ofs.open(std::string("/opt/run/") + program + std::string(".pid"));
    if (ofs.fail()) ZLOG_THROW("WriteProgramPid failed: %s", strerror(errno));
    ofs << pid;
    ofs.close();
    return 0;
}

long GetProgramPid(const std::string& program) {
    long pid = 0;
    std::string filename = std::string("/opt/run/") + program + std::string(".pid");
    if (!IsFileExisted(filename)) return pid;
    std::ifstream ofs;
    ofs.open(filename);
    if (ofs.fail()) ZLOG_THROW("GetProgramPid failed: %s", strerror(errno));
    ofs >> pid;
    ofs.close();
    return pid;
}

bool EnsureOneInstance(const std::string& program) {
    int currentPid = getpid();
    long filePid = GetProgramPid(program);
    if (currentPid == filePid) return true;
    if (!IsProcessExist(filePid)) return true;
    string fileCmdLine = GetProcCmdline(filePid);
    string cmdLine = GetProcCmdline(currentPid);
    return cmdLine != fileCmdLine;
}

bool EnsureOneInstanceFromRun(const std::string& program) {
    std::string filename = std::string("/opt/run/") + program + std::string(".run");
    if (!IsFileExisted(filename)) return true;
    INIReader reader(filename);
    auto pid = reader.GetIntegerOrThrow(program, "process");
    if (!IsProcessExist(pid)) return true;
    std::string cmdline = reader.GetOrThrow(program, "cmdline");
    auto cmdline_ = GetProcCmdline(pid);
    return !(cmdline_ == cmdline);
}
}
