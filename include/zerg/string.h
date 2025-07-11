#pragma once

#include <cstring>
#include <string>
#include <vector>
#include <sstream>
#include <map>

using std::string;

namespace zerg {
std::vector<std::string> split(const std::string& str, char delimiter = ' ');
std::pair<std::string, std::string> SplitInstrumentID(const std::string& str);
std::string gbk2utf(const char* source);
std::string gbk2utf(const std::string& str);
std::string ToUpperCaseCopy(const std::string& s);
std::string ToLowerCaseCopy(const std::string& s);
std::string ToUpperCase(std::string& str);
std::string ToLowerCase(std::string& str);
void ReplaceAll(std::string& str, const std::string& from, const std::string& to);
std::string ReplaceAllCopy(const std::string& str, const std::string& from, const std::string& to);
std::string& LTrim(std::string& s);
std::string& RTrim(std::string& s);
std::string& Trim(std::string& s);
std::string LTrimCopy(const std::string& s);
std::string RTrimCopy(const std::string& s);
std::string TrimCopy(const std::string& s);
void text_word_warp(const std::string& text, size_t maxLineSize, std::vector<std::string>& lines);
std::string get_bool_string(bool value);
std::string get_bool_string(int value);
bool start_with(const std::string& s, const std::string& p);
bool end_with(const std::string& s, const std::string& p);
std::string GenerateRandomString(size_t length, uint32_t seed = 0);
std::vector<std::string> expand_names(const std::string& str);
std::string to_string_high_precision(double value);
std::string ReplaceStringHolderCopy(const std::string& str, const std::map<std::string, std::string>& holders);
std::string ReplaceStringHolder(std::string& str, const std::map<std::string, std::string>& holders);
std::string ReplaceSpecialTimeHolderCopy(const std::string& str);
std::string ReplaceSpecialTimeHolder(std::string& str);
std::string ReplaceSpecialTimeHolderCopy(const std::string& str, const std::string& datetime, const std::string& format);
std::string ReplaceSpecialTimeHolder(std::string& str, const std::string& datetime, const std::string& format);
std::string read_file(const std::string& path);

template <class T>
std::string string_join(const std::vector<T>& v, char delimiter = ' ') {
    std::ostringstream os;
    if (!v.empty()) {
        os << v[0];
        for (size_t i = 1; i < v.size(); ++i) {
            os << delimiter << v[i];
        }
    }
    return os.str();
}
}
