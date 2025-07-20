
#include <iconv.h>
#include <algorithm>
#include <bitset>
#include <cctype>
#include <cstdint>
#include <functional>
#include <iostream>
#include <locale>
#include <memory>
#include <random>
#include <sstream>
#include <zerg/string.h>
#include <zerg/time/clock.h>

using std::string;

namespace zerg {
int code_convert(const char* from_charset, const char* to_charset, char* inBuf, size_t inLen, char* outBuf,
                        size_t outLen) {
    iconv_t cd;
    char** pin = &inBuf;
    char** pout = &outBuf;

    cd = iconv_open(to_charset, from_charset);
    if (cd == nullptr) return -1;
    memset(outBuf, 0, outLen);
    if ((int)iconv(cd, pin, &inLen, pout, &outLen) == -1) return -1;
    iconv_close(cd);
    **pout = '\0';
    return 0;
}

std::string gbk2utf(const char* source) {
    char origin[128], converted[128];
    strcpy(origin, source);
    code_convert((char*)"gbk", (char*)"utf-8", origin, strlen(source), converted, 128);
    return std::string(converted);
}

std::string gbk2utf(const std::string& str) { return gbk2utf(str.c_str()); }

std::string ToUpperCaseCopy(const std::string& s) {
    std::string str = s;
    std::transform(str.begin(), str.end(), str.begin(), ::toupper);
    return str;
}

std::string ToLowerCaseCopy(const std::string& s) {
    std::string str = s;
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    return str;
}

std::string ToUpperCase(std::string& str) {
    std::transform(str.begin(), str.end(), str.begin(), ::toupper);
    return str;
}

std::string ToLowerCase(std::string& str) {
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    return str;
}

void ReplaceAll(std::string& str, const std::string& from, const std::string& to) {
    if (from.empty() || str.empty()) return;
    auto start_pos = str.find(from);
    while (start_pos != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos = str.find(from, start_pos + to.length());  //+to.length in case 'to' contains 'from'
    }
}

std::string ReplaceAllCopy(const std::string& str, const std::string& from, const std::string& to) {
    std::string ret = str;
    ReplaceAll(ret, from, to);
    return ret;
}

// trim from start
std::string& LTrim(std::string& s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
    return s;
}

// trim from end
std::string& RTrim(std::string& s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
    return s;
}

// trim from both ends
std::string& Trim(std::string& s) { return LTrim(RTrim(s)); }

// trim from start
std::string LTrimCopy(const std::string& s) {
    auto str = s;
    str.erase(str.begin(), std::find_if(str.begin(), str.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
    return str;
}

// trim from end
std::string RTrimCopy(const std::string& s) {
    auto str = s;
    str.erase(std::find_if(str.rbegin(), str.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(),
              str.end());
    return str;
}

// trim from both ends
std::string TrimCopy(const std::string& s) {
    auto str = s;
    str.erase(std::find_if(str.rbegin(), str.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(),
              str.end());
    str.erase(str.begin(), std::find_if(str.begin(), str.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
    return str;
}

void text_word_warp(const std::string& text, size_t maxLineSize, std::vector<std::string>& lines) {
    maxLineSize = std::max(maxLineSize, (size_t)16);
    size_t remaining = text.size();
    size_t textSize = text.size();
    size_t start{0};
    const char* pData = text.c_str();

    while (remaining > maxLineSize) {
        size_t end = start + maxLineSize;
        // find last whitespace
        while (!isspace(pData[end - 1] && end > start)) end--;
        // find last non whitespace
        while (isspace(pData[end - 1] && end > start)) end--;

        bool truncate = false;
        if (end == start) {
            end = start + maxLineSize - 1;
            truncate = true;
        }

        std::string line(text.substr(start, end - start));
        if (truncate) line += '-';

        lines.push_back(line);
        start = end;

        while (isspace(pData[start]) && start < textSize) ++start;
        remaining = textSize - start;
    }

    if (remaining > 0) {
        lines.push_back(text.substr(start, remaining));
    }
}

std::string get_bool_string(int value) {
    if (value)
        return "true";
    else
        return "false";
}

std::vector<std::string> split(const std::string& str, char delimiter) {
    std::vector<std::string> result;
    size_t start = 0;
    size_t end = str.find(delimiter);
    while (end != std::string::npos) {
        result.push_back(str.substr(start, end - start));
        start = end + 1;
        end = str.find(delimiter, start);
    }
    result.push_back(str.substr(start, end));
    return result;
}

std::pair<std::string, std::string> SplitInstrumentID(const std::string& str) {
    std::pair<std::string, std::string> ret;
    auto idx = str.find('.');
    if (idx == std::string::npos) {
        ret.first = str;
    } else {
        ret.first = str.substr(0, idx);
        ret.second = str.substr(idx + 1);
    }
    return ret;
}

bool start_with(const std::string& s, const std::string& p) { return s.find(p) == 0; }
bool end_with(const std::string& s, const std::string& p) {
    if (s.length() >= p.length()) {
        return (0 == s.compare(s.length() - p.length(), p.length(), p));
    } else {
        return false;
    }
}

std::string GenerateRandomString(size_t length, uint32_t seed) {
    if (!seed) {  // if seed is zero, will init seed
        std::random_device rd;
        seed = rd();
    }
    std::mt19937 generator(seed);

    char* s = (char*)malloc((length + 1) * sizeof(char));
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";

    std::uniform_int_distribution<int> uid(0, sizeof(alphanum) - 2);
    for (size_t i = 0; i < length; ++i) {
        s[i] = alphanum[uid(generator)];
    }

    s[length] = '\0';
    auto ret = std::string(s);
    free(s);
    return ret;
}

std::vector<std::string> expand_names(const std::string& str) {
    std::vector<std::string> ret;
    auto name_patterns = zerg::split(str, ',');
    for (auto& pattern : name_patterns) {
        if (pattern.empty()) continue;
        auto p1 = pattern.find('[');
        if (p1 == std::string::npos) {
            ret.push_back(pattern);
        } else {
            auto p2 = pattern.find(']');
            if (p2 == string::npos || p2 < p1) {
                ret.push_back(pattern);
            } else {
                string prefix = pattern.substr(0, p1);
                string suffix = pattern.substr(p2 + 1);
                string wildcards = pattern.substr(p1 + 1, p2 - p1 - 1);
                if (wildcards.find('-') != string::npos) {
                    auto lets = zerg::split(wildcards, '-');
                    if (lets.size() == 2) {
                        int start_idx = std::stoi(lets[0]);
                        int end_idx = std::stoi(lets[1]);
                        for (int i = start_idx; i <= end_idx; ++i) {
                            ret.push_back(prefix + std::to_string(i) + suffix);
                        }
                    } else {
                        ret.push_back(pattern);
                    }
                } else {
                    ret.push_back(pattern);
                }
            }
        }
    }
    return ret;
}

std::string to_string_high_precision(double value) {
    char buffer[64];
    memset(buffer, 0, sizeof(buffer));
    snprintf(buffer, sizeof(buffer), "%g", value);
    return std::string(buffer);
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

std::string ReplaceSpecialTimeHolderCopy(const std::string& str, const Clock& clock) {
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

std::string ReplaceSpecialTimeHolder(std::string& str, const Clock& clock) {
    auto ret = ReplaceSpecialTimeHolderCopy(str, clock);
    str = ret;
    return ret;
}

std::string ReplaceSpecialTimeHolderCopy(const std::string& str) {
    Clock clock;  // use today as default
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
    Clock clock(datetime.c_str(), format.c_str());  // use today as default

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
std::vector<std::string> read_file_lines(const std::string &config) {
  std::vector<std::string> lines;
  std::string iline;

  std::ifstream ifile(config);
  if (!ifile.is_open()) {
    throw std::runtime_error("invalid " + config);
  }
  while(std::getline(ifile, iline)) {
    lines.push_back(iline);
  }
  return lines;
}
std::string read_file(const std::string& path) {
    std::ifstream ifile(path);
    if (!ifile.good()) {
        ZLOG_THROW("invalid input %s", path.c_str());
    }
    std::ostringstream ss;
    ss << ifile.rdbuf();
    return ss.str();
}
bool write_file(const std::string& path, const std::string& content) {
    std::ofstream ofile(path, std::ios::trunc);
    if (ofile.is_open() == false) {
        ZLOG_THROW("file %s open failed", path.c_str());
    }
    ofile << content;
    ofile.close();
    return true;
}
}
