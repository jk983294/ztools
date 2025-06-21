#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <regex>

namespace zerg {
struct OutputColumnOption {
    int type{0};  // 1: double, 2:float, 3:int, 4:string, 5:bool, 6:uint64_t
    void* data{nullptr};
    std::string name;
};

struct InputData {
    InputData() = default;
    ~InputData();

    void clear();

    template<typename T>
    std::vector<T>* new_type_vec(std::vector<std::vector<T>*>& pool, std::vector<std::vector<T>*>& pool1, uint64_t size) {
        if (pool.empty()) {
            auto ptr = new std::vector<T>(size);
            pool1.push_back(ptr);
            return ptr;
        } else {
            auto* pFeature = pool.back();
            pool.pop_back();
            pool1.push_back(pFeature);
            pFeature->clear();
            pFeature->resize(size);
            return pFeature;
        }
    }

    std::vector<double>* new_double_vec(uint64_t size) { return new_type_vec<double>(m_double_pool, doubles, size); }
    std::vector<float>* new_float_vec(uint64_t size) { return new_type_vec<float>(m_float_pool, floats, size); }
    std::vector<int>* new_int_vec(uint64_t size) { return new_type_vec<int>(m_int_pool, ints, size); }
    std::vector<uint64_t>* new_uint64_vec(uint64_t size) { return new_type_vec<uint64_t>(m_uint64_pool, uint64s, size); }
    std::vector<bool>* new_bool_vec(uint64_t size) { return new_type_vec<bool>(m_bool_pool, bools, size); }
    std::vector<std::string>* new_str_vec(uint64_t size) { return new_type_vec<std::string>(m_str_pool, strs, size); }

    std::vector<std::vector<int>*> ints;
    std::vector<std::vector<uint64_t>*> uint64s;
    std::vector<std::vector<double>*> doubles;
    std::vector<std::vector<float>*> floats;
    std::vector<std::vector<bool>*> bools;
    std::vector<std::vector<std::string>*> strs;
    std::vector<OutputColumnOption> cols;
    uint64_t rows{0};

    std::vector<std::vector<int>*> m_int_pool;
    std::vector<std::vector<uint64_t>*> m_uint64_pool;
    std::vector<std::vector<double>*> m_double_pool;
    std::vector<std::vector<float>*> m_float_pool;
    std::vector<std::vector<bool>*> m_bool_pool;
    std::vector<std::vector<std::string>*> m_str_pool;
};

struct DayData {
    std::vector<int>* x_ukeys{nullptr};
    std::vector<int>* x_dates{nullptr};
    std::vector<int>* x_ticks{nullptr};
    std::vector<std::vector<double>*> pXs;
    std::vector<std::string> xNames;
    std::vector<double> y;
    std::vector<bool> y_tradable;
    std::unordered_map<int, std::unordered_map<int, uint64_t>> ukey2tick2pos;
    std::unordered_map<int, std::vector<uint64_t>> tick2pos;
    std::vector<int> sorted_ticks;
    std::unordered_map<std::string, double*> x2data;
    std::unordered_map<std::string, std::vector<bool>*> x2bool_data;
    InputData id;
    bool m_meta_column_only{false};
    bool m_add_ticktime_for_daily{false};

    void build_index();
    bool add_new_column(std::string col);
};

struct DailyDatum {
    std::string key;
    std::string input_folder;
    bool skip_ticktime;
    bool skip_date_check;
    bool use_prev_date_data;
    std::string input_format;
    int nday{0};
    std::unordered_map<int, std::shared_ptr<DayData>> m_dds;
};

std::shared_ptr<DayData> load_feather_data(const std::string& input_file);

struct X_Data {
    bool m_read_meta_only{false};
    std::string x_path_pattern;
    std::string m_x_pattern;
    std::unordered_map<std::string, bool> m_x_names; // to read columns
    std::unordered_map<int, std::string> m_x2files;
    InputData id;
    InputData mid;
    X_Data();
    void clear();
    uint64_t read(DayData& d, std::string path);
    uint64_t read_csv(DayData& d, std::string path);
    void merge_read(DayData& d, std::string path);
    void set_x_path_pattern(std::string path);
    bool check_missing(const std::vector<int>& dates) const;
    bool is_wanted_column(std::string col, const std::regex& x_regex) const;
};
}