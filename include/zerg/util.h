#pragma once

#include <sched.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <map>
#include <random>
#include <set>
#include <string>
#include <zerg/io/IniReader.h>
#include <zerg/io/file.h>
#include <zerg/macro.h>
#include <zerg/string.h>
#include <zerg/time/time.h>

namespace zerg {
class BashCommand {
    const int READ = 0;
    const int WRITE = 1;

public:
    FILE* pFile;
    pid_t pid;
    std::string m_command;
    std::string m_type;

    BashCommand(std::string command, std::string type) {
        m_command = command;
        m_type = type;
    }

    FILE* Run() {
        pid_t child_pid;
        int fd[2];
        pipe(fd);

        if ((child_pid = fork()) == -1) {
            perror("fork");
            exit(1);
        }

        if (child_pid == 0) {  // child
            if (m_type == "r") {
                close(fd[READ]);     // Close the READ end of the pipe since the child's fd is write-only
                dup2(fd[WRITE], 1);  // Redirect stdout to pipe
            } else {
                close(fd[WRITE]);   // Close the WRITE end of the pipe since the child's fd is read-only
                dup2(fd[READ], 0);  // Redirect stdin to pipe
            }

            setpgid(child_pid, child_pid);  // Needed so negative PIDs can kill children of /bin/sh
            execl("/bin/sh", "/bin/sh", "-c", m_command.c_str(), nullptr);
            exit(0);
        } else {
            if (m_type == "r") {
                close(fd[WRITE]);  // Close the WRITE end of the pipe since parent's fd is read-only
            } else {
                close(fd[READ]);  // Close the READ end of the pipe since parent's fd is write-only
            }
        }

        pid = child_pid;
        if (m_type == "r") {
            pFile = fdopen(fd[READ], "r");
            return pFile;
        }
        pFile = fdopen(fd[WRITE], "w");
        return pFile;
    }

    int Shutdown() {
        int stat;
        fclose(pFile);
        kill(pid, SIGKILL);
        while (waitpid(pid, &stat, 0) == -1) {
            if (errno != EINTR) {
                stat = -1;
                break;
            }
        }
        return stat;
    }
};

int EnableCoreDump() {
    struct rlimit core_limits;
    core_limits.rlim_cur = core_limits.rlim_max = RLIM_INFINITY;
    setrlimit(RLIMIT_CORE, &core_limits);
    return 0;
}

int DisableCoreDump() {
    struct rlimit core_limits;
    core_limits.rlim_cur = core_limits.rlim_max = 0;
    setrlimit(RLIMIT_CORE, &core_limits);
    return 0;
}

int WriteProgramPid(const std::string& program) {
    pid_t pid = getpid();
    std::ofstream ofs;
    ofs.open(std::string("/opt/run/") + program + std::string(".pid"));
    if (ofs.fail()) THROW_ZERG_EXCEPTION(std::string("WriteProgramPid failed: ") << strerror(errno));
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
    if (ofs.fail()) THROW_ZERG_EXCEPTION(std::string("GetProgramPid failed: ") << strerror(errno));
    ofs >> pid;
    ofs.close();
    return pid;
}

bool IsProcessExist(long pid) {
    struct stat sts;
    std::string path = "/proc/" + std::to_string((long long)pid);
    return !(stat(path.c_str(), &sts) == -1 && errno == ENOENT);
}

std::string GetProcCmdline(pid_t process) {
    std::string ret;
    unsigned char buffer[4096];
    char filename[128];
    memset(filename, 0, 128);
    sprintf(filename, "/proc/%d/cmdline", process);
    int fd = open(filename, O_RDONLY);
    auto nBytesRead = read(fd, buffer, 4096);
    unsigned char* end = buffer + nBytesRead;
    for (unsigned char* p = buffer; p < end;) {
        if (p == buffer)
            ret += std::string((const char*)p);
        else
            ret += " " + std::string((const char*)p);
        while (*p++)
            ;
    }
    close(fd);
    return ret;
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

bool EnsureOneInstanceFromXml(const std::string& program, const std::string xml) {
    std::string filename = std::string("/opt/run/") + program + std::string(".run");
    if (!IsFileExisted(filename)) return true;
    INIReader reader(filename);
    auto pid = reader.GetIntegerOrThrow(program, "process");
    if (!IsProcessExist(pid)) return true;
    std::string xmlFile = reader.GetOrThrow(program, "xml");
    std::string realXml = GetAbsolutePath(xml);
    return !(realXml == xmlFile);
}

void BindCore(size_t cpu_id) {
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(cpu_id, &mask);
    if (sched_setaffinity(0, sizeof(mask), &mask) != 0) THROW_ZERG_EXCEPTION("CPU affinity setting failed.");
}

void BindCore(std::vector<size_t>& cpu_id_list) {
    cpu_set_t mask;
    CPU_ZERO(&mask);
    for (auto node : cpu_id_list) {
        CPU_SET(node, &mask);
    }
    if (sched_setaffinity(0, sizeof(mask), &mask) != 0) THROW_ZERG_EXCEPTION("CPU affinity setting failed.");
}

// get cpu cycles counts
uint64_t GetRDTSC() {
    unsigned int lo, hi;
    __asm(
        "XOR %eax, %eax\t\n"
        "CPUID\t\n"
        "XOR %eax, %eax\t\n"
        "CPUID\t\n"
        "XOR %eax, %eax\t\n"
        "CPUID\t\n");
    asm volatile("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}

std::string ReplaceStringHolderCopy(const std::string& str, const std::map<std::string, std::string>& holders) {
    std::string ret = str;
    std::string holder;
    for (const auto& iter : holders) {
        holder = "${" + iter.first + "}";
        ReplaceAll(ret, holder, iter.second);
    }
    return ret;
}

std::string ReplaceStringHolder(std::string& str, const std::map<std::string, std::string>& holders) {
    std::string holder;
    for (const auto& iter : holders) {
        holder = "${" + iter.first + "}";
        ReplaceAll(str, holder, iter.second);
    }
    return str;
}

template <typename T>
std::string ReplaceSpecialTimeHolderCopy(const std::string& str, const Clock<T>& clock) {
    std::string year, month, day, timestamp, timestampsss;
    year = clock.YearToStr();
    month = clock.MonthToStr();
    day = clock.DayToStr();

    timestamp = clock.HourToStr() + clock.MinuteToStr() + clock.SecondToStr();
    timestampsss = timestamp + clock.MillisecondToStr();

    std::string ret = str;
    ReplaceAll(ret, "${YYYY}", year);
    ReplaceAll(ret, "${MM}", month);
    ReplaceAll(ret, "${DD}", day);
    ReplaceAll(ret, "${YYYYMMDD}", year + month + day);
    ReplaceAll(ret, "${YYYYMM}", year + month);
    ReplaceAll(ret, "${MMDD}", month + day);
    ReplaceAll(ret, "${HHMMSS}", timestamp);
    ReplaceAll(ret, "${HHMMSSmmm}", timestampsss);
    return ret;
}

template <typename T>
std::string ReplaceSpecialTimeHolder(std::string& str, const Clock<T>& clock) {
    auto ret = ReplaceSpecialTimeHolderCopy(str, clock);
    str = ret;
    return ret;
}

std::string ReplaceSpecialTimeHolderCopy(const std::string& str) {
    Clock<> clock;  // use today as default
    clock.Update();
    std::string year, month, day, timestamp, timestampsss;
    year = clock.YearToStr();
    month = clock.MonthToStr();
    day = clock.DayToStr();

    timestamp = clock.HourToStr() + clock.MinuteToStr() + clock.SecondToStr();
    timestampsss = timestamp + clock.MillisecondToStr();

    std::string ret = str;
    ReplaceAll(ret, "${YYYY}", year);
    ReplaceAll(ret, "${MM}", month);
    ReplaceAll(ret, "${DD}", day);
    ReplaceAll(ret, "${YYYYMMDD}", year + month + day);
    ReplaceAll(ret, "${YYYYMM}", year + month);
    ReplaceAll(ret, "${MMDD}", month + day);
    ReplaceAll(ret, "${HHMMSS}", timestamp);
    ReplaceAll(ret, "${HHMMSSmmm}", timestampsss);
    return ret;
}

std::string ReplaceSpecialTimeHolderCopy(const std::string& str, const std::string& datetime,
                                                const std::string& format) {
    Clock<> clock(datetime.c_str(), format.c_str());  // use today as default

    std::string year, month, day, timestamp, timestampsss;
    year = clock.YearToStr();
    month = clock.MonthToStr();
    day = clock.DayToStr();

    timestamp = clock.HourToStr() + clock.MinuteToStr() + clock.SecondToStr();
    timestampsss = timestamp + clock.MillisecondToStr();

    std::string ret = str;
    ReplaceAll(ret, "${YYYY}", year);
    ReplaceAll(ret, "${MM}", month);
    ReplaceAll(ret, "${DD}", day);
    ReplaceAll(ret, "${YYYYMMDD}", year + month + day);
    ReplaceAll(ret, "${YYYYMM}", year + month);
    ReplaceAll(ret, "${MMDD}", month + day);
    ReplaceAll(ret, "${HHMMSS}", timestamp);
    ReplaceAll(ret, "${HHMMSSmmm}", timestampsss);
    return ret;
}

std::string ReplaceSpecialTimeHolder(std::string& str) {
    auto ret = ReplaceSpecialTimeHolderCopy(str);
    str = ret;
    return ret;
}

std::string ReplaceSpecialTimeHolder(std::string& str, const std::string& datetime, const std::string& format) {
    auto ret = ReplaceSpecialTimeHolderCopy(str, datetime, format);
    str = ret;
    return ret;
}

std::string GetUserName() {
    char name[200];
    getlogin_r(name, 199);
    return std::string(name);
}

std::string GetProgramPath() {
    char name[256];
    auto r = readlink("/proc/self/exe", name, sizeof(name));
    if (r != -1) name[r] = '\0';
    return std::string(name);
}
}
