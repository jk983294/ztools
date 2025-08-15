#include <zerg/io/json_util.h>
#include <zerg/io/file.h>
#include <fstream>

namespace zerg {
nlohmann::json json_open(const std::string& path) {
    std::ifstream ifs(path);
    std::string wholeFileStr((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    return nlohmann::json::parse(wholeFileStr);
}
}