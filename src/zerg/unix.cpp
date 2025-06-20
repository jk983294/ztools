#include <stdlib.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctime>
#include <fstream>
#include <ios>
#include <iostream>
#include <cstring>
#include <zerg/unix.h>
#include <zerg/log.h>

namespace zerg {

bool CheckProcessAlive(pid_t id) {
    if (id <= 0) return false;
    while (waitpid(-1, nullptr, WNOHANG) > 0) {
    }  // Wait for defunct, zombie sub-process get collected
    return 0 == kill(id, 0);
}

void MemUsage(double &vm_usage, double &resident_set) {
    using std::ifstream;
    using std::ios_base;
    using std::string;

    vm_usage = 0.0;
    resident_set = 0.0;
    ifstream stat_stream("/proc/self/stat", ios_base::in);
    string pid, comm, state, parentPid, procGroup, session, tty;
    string terminalGroupId, flags, minflt, cminflt, majflt, cmajflt;
    string userTime, systemTime, childUserTime, childSystemTime, priority, nice;
    string numThreads, itrealvalue, starttime;

    unsigned long vsize;  // virtual memory size in bytes
    long rss;             // resident set size: number of pages the process has in real memory

    stat_stream >> pid >> comm >> state >> parentPid >> procGroup >> session >> tty >> terminalGroupId >> flags >>
        minflt >> cminflt >> majflt >> cmajflt >> userTime >> systemTime >> childUserTime >> childSystemTime >>
        priority >> nice >> numThreads >> itrealvalue >> starttime >> vsize >> rss;  // don't care about the rest

    stat_stream.close();

    long page_size_kb = sysconf(_SC_PAGE_SIZE) / 1024;  // in case x86-64 is configured to use 2MB pages
    vm_usage = vsize / 1024.0;
    resident_set = rss * page_size_kb;
}

std::vector<size_t> GetAffinity(pthread_t id) {
    std::vector<size_t> ret;
    cpu_set_t cpuSet;
    CPU_ZERO(&cpuSet);
    if (id == 0) id = pthread_self();
    auto s = pthread_getaffinity_np(id, sizeof(cpu_set_t), &cpuSet);
    (void)s;
    for (size_t j = 0; j < CPU_SETSIZE; j++) {
        if (CPU_ISSET(j, &cpuSet)) ret.push_back(j);
    }
    return ret;
}

std::string GetHostName() {
    std::string hostname(1024, '\0');
    gethostname((char *)hostname.data(), hostname.capacity());

    auto pos = hostname.find('.');
    if (pos != std::string::npos) {
        return hostname.substr(0, pos);
    } else
        return hostname;
}

std::string GetDomainName() {
    std::string hostname(1024, '\0');
    gethostname((char *)hostname.data(), hostname.capacity());

    auto pos = hostname.find('.');
    if (pos != std::string::npos) {
        return hostname.substr(pos + 1, hostname.size() - pos);
    } else
        return "";
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

void BindCore(size_t cpu_id) {
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(cpu_id, &mask);
    if (sched_setaffinity(0, sizeof(mask), &mask) != 0) ZLOG_THROW("CPU affinity setting failed.");
}

void BindCore(std::vector<size_t>& cpu_id_list) {
    cpu_set_t mask;
    CPU_ZERO(&mask);
    for (auto node : cpu_id_list) {
        CPU_SET(node, &mask);
    }
    if (sched_setaffinity(0, sizeof(mask), &mask) != 0) ZLOG_THROW("CPU affinity setting failed.");
}


void System::Init() {
    struct timespec start;
    clock_gettime(CLOCK_REALTIME, &start);
    start_microsecond = start.tv_sec * 1000 * 1000 + start.tv_nsec / 1000;
    process = getpid();
    cmd = GetProgramPath();
    user = GetUserName();
    comm = GetProcCmdline(process);
}

}
