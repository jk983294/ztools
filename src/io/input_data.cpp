#include <zerg/io/input_data.h>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <zerg/io/feather_helper.h>
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
}