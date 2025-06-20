#pragma once

#include <pthread.h>
#include <string>
#include <vector>

namespace zerg {

inline void Escape(void *p) { asm volatile("" : : "g"(p) : "memory"); }

bool CheckProcessAlive(pid_t id);
void MemUsage(double &vm_usage, double &resident_set);
std::vector<size_t> GetAffinity(pthread_t id = 0);
std::string GetHostName();
std::string GetDomainName();

struct System {
    std::string program;
    int process;
    std::string cfg_path;
    std::string ID;
    std::string user;
    long start_microsecond;
    std::string log_file;
    std::string cmd;
    std::string comm;

    void Init();
};

}
