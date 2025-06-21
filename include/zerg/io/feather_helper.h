#pragma once

#include <zerg/io/input_data.h>
#include <regex>

namespace zerg {
struct FeatherReader {
    FeatherReader() = default;
    static void read(std::string path_, InputData& id, const std::string& x_pattern = "",
        const std::unordered_map<std::string, bool>& x_names = {}, bool metaOnly = false);
};

bool write_feather(std::string path_, size_t nrow, const std::vector<OutputColumnOption>& cols, bool use_float32 = false);
bool write_feather(std::string path_, size_t nrow,
    const std::vector<std::string>& int_col_names, const std::vector<std::string>& double_col_names,
    const std::vector<const int*>& int_cols, const std::vector<const double*>& double_cols, bool use_float32 = false);
bool write_feather(std::string path_, const std::vector<int>& ints, const std::vector<double>& doubles, bool use_float32 = false);
bool write_feather(std::string path_, const std::vector<int>& vs, bool use_float32 = false);
bool write_feather(std::string path_, const std::vector<double>& vs, bool use_float32 = false);
}