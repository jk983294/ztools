#pragma once

#include <string>

namespace zerg {
int WriteProgramPid(const std::string& program);
long GetProgramPid(const std::string& program);
bool EnsureOneInstance(const std::string& program);
bool EnsureOneInstanceFromRun(const std::string& program);
}
