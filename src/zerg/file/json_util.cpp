#include <zerg/io/json_util.h>
#include <zerg/io/file.h>
#include <zerg/string.h>
#include <fstream>

namespace zerg {
nlohmann::json json_open(const std::string& path) {
    std::ifstream ifs(path);
    std::string wholeFileStr((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());

    auto json_config = nlohmann::json::parse(wholeFileStr);

    if (json_config.contains("defines")) {
        std::map<std::string, std::string> cfg_defines;
        for (auto& el : json_config["defines"].items()) {
            cfg_defines[el.key()] = el.value();
        }
        if (!cfg_defines.empty()) {
            printf("replace %s with %zu cfg defines\n", path.c_str(), cfg_defines.size());
            zerg::ReplaceStringHolder(wholeFileStr, cfg_defines);
            json_config = nlohmann::json::parse(wholeFileStr);
        }
    }
    return json_config;
}
}