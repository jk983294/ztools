#pragma once

#include <string>
#include <iosfwd>

namespace zerg {
int WriteProgramPid(const std::string& program);
long GetProgramPid(const std::string& program);
bool EnsureOneInstance(const std::string& program);
bool EnsureOneInstanceFromRun(const std::string& program);
int ExecRedirect(const char* cmd, std::ostream& ofs);
}
