#pragma once
#include <vector>
#include <cstdint>
#include <string>

namespace zerg {
std::string encrypt(const std::string& input, const std::vector<uint8_t>& key, const std::vector<uint8_t>& iv);
std::string decrypt(const std::string& cipher_text, const std::vector<uint8_t>& key, const std::vector<uint8_t>& iv);
std::string encrypt(const std::string& input, const std::string& key);
std::string decrypt(const std::string& cipher_text, const std::string& key);
}
