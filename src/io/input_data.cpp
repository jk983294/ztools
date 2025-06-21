#include <zerg/io/input_data.h>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <zerg/io/feather_helper.h>
#include <zerg/io/csv_helper.h>
#include <zerg/io/file.h>
#include <zerg/log.h>

namespace zerg {
void DayData::build_index() {
    for (uint64_t i = 0; i < x_ukeys->size(); ++i) {
        ukey2tick2pos[(*x_ukeys)[i]][(*x_ticks)[i]] = i;
        tick2pos[(*x_ticks)[i]].push_back(i);
    }

    for (uint64_t i = 0; i < xNames.size(); ++i) {
        x2data[xNames[i]] = pXs[i]->data();
    }

    std::transform(tick2pos.begin(), tick2pos.end(), std::back_inserter(sorted_ticks), [](auto& i) { return i.first; });
    std::sort(sorted_ticks.begin(), sorted_ticks.end());
}

bool DayData::add_new_column(std::string col) {
    auto itr = std::find(xNames.begin(), xNames.end(), col);
    if (itr == xNames.end()) {
        auto* ptr = id.new_double_vec(x_ukeys->size());
        std::fill(ptr->begin(), ptr->end(), NAN);
        pXs.push_back(ptr);
        xNames.push_back(col);
        return true;
    }
    return false;
}

InputData::~InputData() {
    clear();
    for (auto* vec : m_double_pool) delete vec;
    for (auto* vec : m_float_pool) delete vec;
    for (auto* vec : m_int_pool) delete vec;
    for (auto* vec : m_uint64_pool) delete vec;
    for (auto* vec : m_bool_pool) delete vec;
    for (auto* vec : m_str_pool) delete vec;
}

void InputData::clear() {
    m_double_pool.insert(m_double_pool.end(), doubles.begin(), doubles.end());
    m_float_pool.insert(m_float_pool.end(), floats.begin(), floats.end());
    m_int_pool.insert(m_int_pool.end(), ints.begin(), ints.end());
    m_uint64_pool.insert(m_uint64_pool.end(), uint64s.begin(), uint64s.end());
    m_bool_pool.insert(m_bool_pool.end(), bools.begin(), bools.end());
    m_str_pool.insert(m_str_pool.end(), strs.begin(), strs.end());
    doubles.clear();
    ints.clear();
    uint64s.clear();
    bools.clear();
    strs.clear();
    cols.clear();
    rows = 0;
}

std::shared_ptr<DayData> load_feather_data(const std::string& input_file) {
  auto pdd = std::make_shared<DayData>();
  auto& d = *pdd;
  if (!zerg::IsFileExisted(input_file)) {
    ZLOG_THROW("missing daily data %s", input_file.c_str());
  }

  uint64_t rows{0};
  std::vector<OutputColumnOption>* cols{nullptr};
  FeatherReader x_reader1;
  x_reader1.read(input_file, d.id);
  rows = d.id.rows;
  cols = &d.id.cols;

  for (auto& col : *cols) {
    if (col.type == 1) {
      auto& vec = *reinterpret_cast<std::vector<double>*>(col.data);
      d.xNames.push_back(col.name);
      d.pXs.push_back(&vec);
    } else if (col.type == 3) {
      auto& vec = *reinterpret_cast<std::vector<int>*>(col.data);
      if (col.name == "ukey")
        d.x_ukeys = &vec;
      else if (col.name == "ticktime")
        d.x_ticks = &vec;
      else if (col.name == "DataDate")
        d.x_dates = &vec;
    } else if (col.type == 5) {
      auto& vec = *reinterpret_cast<std::vector<bool>*>(col.data);
      d.x2bool_data[col.name] = &vec;
    }
  }

  if (d.x_ukeys == nullptr || d.x_ticks == nullptr || d.x_dates == nullptr) {
      throw std::runtime_error("no ukey/tick/date column " + input_file);
  }
  if (d.xNames.empty()) {
      throw std::runtime_error("empty x column " + input_file);
  }
  if (rows == 0) {
      throw std::runtime_error("empty x table " + input_file);
  }

  d.build_index();

  std::cout << "loaded " + input_file << std::endl;
  return pdd;
}

X_Data::X_Data() {
    m_x_names["ukey"] = true;
    m_x_names["ticktime"] = true;
    m_x_names["DataDate"] = true;
}

void X_Data::clear() {
    x_path_pattern = "";
    m_x_pattern = "";
    m_x_names.clear();
    m_x2files.clear();
    id.clear();
    mid.clear();
}

bool X_Data::check_missing(const std::vector<int>& dates) const {
    for (int d : dates) {
        if (m_x2files.find(d) == m_x2files.end()) {
            // throw std::runtime_error(m_path_pattern + " missing date " + std::to_string(d));
            printf("%s missing date %d\n", m_x_pattern.c_str(), d);
            return false;
        }
    }
    return true;
}

bool X_Data::is_wanted_column(std::string col, const std::regex& x_regex) const {
    bool all_empty = m_x_pattern.empty() && m_x_names.empty();
    bool found_name = (!m_x_names.empty() && m_x_names.find(col) != m_x_names.end());
    bool found_pattern = (!m_x_pattern.empty() && std::regex_search(col, x_regex));

    if (!found_name && !found_pattern && !all_empty) {
        return false;
    }
    return true;
}
uint64_t X_Data::read(DayData& d, std::string path) {
    id.clear();
    uint64_t rows{0};
    std::vector<OutputColumnOption>* cols{nullptr};
    zerg::FeatherReader x_reader1;
    x_reader1.read(path, id, m_x_pattern, m_x_names, m_read_meta_only);
    rows = id.rows;
    cols = &id.cols;
    std::regex x_regex(m_x_pattern);

    for (auto& col : *cols) {
        if (col.type == 1) {
            if (is_wanted_column(col.name, x_regex)) {
                auto& vec = *reinterpret_cast<std::vector<double>*>(col.data);
                d.xNames.push_back(col.name);
                d.pXs.push_back(&vec);
                d.x2data[col.name] = vec.data();
            }
        } else if (col.type == 3) {
            auto& vec = *reinterpret_cast<std::vector<int>*>(col.data);
            if (col.name == "ukey")
                d.x_ukeys = &vec;
            else if (col.name == "ticktime")
                d.x_ticks = &vec;
            else if (col.name == "DataDate")
                d.x_dates = &vec;
            else if (is_wanted_column(col.name, x_regex)) {
                auto& vec1 = *id.new_double_vec(rows);
                std::copy(vec.begin(), vec.end(), vec1.begin());
                d.xNames.push_back(col.name);
                d.pXs.push_back(&vec1);
                d.x2data[col.name] = vec1.data();
            }
        } else if (col.type == 5) {
            if (is_wanted_column(col.name, x_regex)) {
                auto& vec = *reinterpret_cast<std::vector<bool>*>(col.data);
                d.x2bool_data[col.name] = &vec;
            }
        }
    }

    if (d.x_ukeys == nullptr || d.x_ticks == nullptr || d.x_dates == nullptr) {
        throw std::runtime_error("no ukey/tick/date column " + path);
    }
    if (d.xNames.empty() && !d.m_meta_column_only) {
        throw std::runtime_error("empty x column " + path);
    }
    if (rows == 0) {
        printf("WARN! empty x table %s\n", path.c_str());
    }
    return rows;
}

uint64_t X_Data::read_csv(DayData& d, std::string path) {
    id.clear();
    uint64_t rows{0};
    std::vector<OutputColumnOption>* cols{nullptr};
    zerg::CsvReader::read(path, id, m_x_pattern, m_x_names);
    rows = id.rows;
    cols = &id.cols;
    std::regex x_regex(m_x_pattern);

    for (auto& col : *cols) {
        if (col.type == 1) {
            if (is_wanted_column(col.name, x_regex)) {
                auto& vec = *reinterpret_cast<std::vector<double>*>(col.data);
                d.xNames.push_back(col.name);
                d.pXs.push_back(&vec);
                d.x2data[col.name] = vec.data();
            }
        } else if (col.type == 3) {
            auto& vec = *reinterpret_cast<std::vector<int>*>(col.data);
            if (col.name == "ukey")
                d.x_ukeys = &vec;
            else if (col.name == "ticktime")
                d.x_ticks = &vec;
            else if (col.name == "DataDate")
                d.x_dates = &vec;
            else if (is_wanted_column(col.name, x_regex)) {
                auto& vec1 = *id.new_double_vec(rows);
                std::copy(vec.begin(), vec.end(), vec1.begin());
                d.xNames.push_back(col.name);
                d.pXs.push_back(&vec1);
                d.x2data[col.name] = vec1.data();
            }
        } else if (col.type == 5) {
            if (is_wanted_column(col.name, x_regex)) {
                auto& vec = *reinterpret_cast<std::vector<bool>*>(col.data);
                d.x2bool_data[col.name] = &vec;
            }
        }
    }

    if (d.x_ticks == nullptr) {
        if (rows < 10000) {
            d.x_ticks = id.new_int_vec(rows);
            std::fill(d.x_ticks->begin(), d.x_ticks->end(), 150000000);
            d.m_add_ticktime_for_daily = true;
        }
    }

    if (d.x_ukeys == nullptr || d.x_ticks == nullptr || d.x_dates == nullptr) {
        throw std::runtime_error("no ukey/tick/date column " + path);
    }
    if (d.xNames.empty() && !d.m_meta_column_only) {
        throw std::runtime_error("empty x column " + path);
    }
    if (rows == 0) {
        printf("WARN! empty x table %s\n", path.c_str());
    }
    return rows;
}

void X_Data::set_x_path_pattern(std::string path) {
    x_path_pattern = path;
}
void X_Data::merge_read(DayData& d, std::string path) {
    id.clear();
    uint64_t rows{0};
    std::vector<OutputColumnOption>* cols{nullptr};
    zerg::FeatherReader x_reader1;
    x_reader1.read(path, id);
    rows = id.rows;
    cols = &id.cols;
    std::regex x_regex(m_x_pattern);

    std::vector<int>* _ukeys{nullptr};
    std::vector<int>* _dates{nullptr};
    std::vector<int>* _ticks{nullptr};
    std::vector<OutputColumnOption> to_merges;
    for (auto& col : *cols) {
        if (col.type == 1) {
            if (m_x_names.find(col.name) != m_x_names.end() || (!m_x_pattern.empty() && std::regex_search(col.name, x_regex))) {
                if (std::find(d.xNames.begin(), d.xNames.end(), col.name) != d.xNames.end()) {
                    throw std::runtime_error("dupe column " + col.name + " in " + path);
                }
                to_merges.push_back(col);
            }
        } else if (col.type == 3) {
            auto& vec = *reinterpret_cast<std::vector<int>*>(col.data);
            if (col.name == "ukey")
                _ukeys = &vec;
            else if (col.name == "ticktime")
                _ticks = &vec;
            else if (col.name == "DataDate")
                _dates = &vec;
        }
    }

    if (_ukeys == nullptr || _ticks == nullptr || _dates == nullptr) {
        throw std::runtime_error("no ukey/tick/date column in " + path);
    }
    if (to_merges.empty()) {
        throw std::runtime_error("empty to_merges x column " + path);
    }
    if (rows == 0) {
        printf("WARN! empty to_merges x table %s\n", path.c_str());
        return;
    }

    mid.clear();
    for (auto& col : to_merges) {
        auto& vec = *reinterpret_cast<std::vector<double>*>(col.data);
        auto pVec = mid.new_double_vec(0);
        pVec->resize(d.x_dates->size(), NAN);
        uint64_t merge_cnt = 0;
        for (uint64_t i = 0; i < _ukeys->size(); ++i) {
            auto itr1 = d.ukey2tick2pos.find((*_ukeys)[i]);
            if (itr1 == d.ukey2tick2pos.end()) continue;
            auto& tick2pos = itr1->second;
            auto itr2 = tick2pos.find((*_ticks)[i]);
            if (itr2 == tick2pos.end()) continue;
            (*pVec)[itr2->second] = vec[i];
            merge_cnt++;
        }
        d.xNames.push_back(col.name);
        d.pXs.push_back(pVec);
        // printf("merge %s %zu,%zu\n", col.name.c_str(), merge_cnt, _ukeys->size());
    }
}
}