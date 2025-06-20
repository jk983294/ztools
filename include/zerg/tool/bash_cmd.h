#pragma once

#include <pthread.h>
#include <string>

using namespace std;

namespace zerg {
struct BashCommand {
    const int READ = 0;
    const int WRITE = 1;

    FILE* pFile;
    pid_t pid;
    std::string m_command;
    std::string m_type;

    BashCommand(std::string command, std::string type);

    FILE* Run();

    int Shutdown();
};
}
