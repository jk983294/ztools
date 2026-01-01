#include <zerg/io/input_data.h>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <zerg/io/feather_helper.h>
#include <zerg/io/csv_helper.h>
#include <zerg/io/file.h>
#include <zerg/log.h>
#include <zerg/string.h>
#include <zerg/time/bizday.h>
#include <zerg/time/dtu.h>

namespace zerg {
void LoadDailyDatum(DailyDatum& item, const std::vector<int32_t>& dates, const std::unordered_map<std::string, bool>& ignore_cols, int print_every_n) {
  int n_day = dates.size();
  for (int di = 0; di < n_day; di++) {
    int d_ = dates[di];
    bool should_print = true;
    if (print_every_n > 0) should_print = (di % print_every_n == 0);
    if (item.input_format == "feather") {
      auto input_file = path_join(item.input_folder, std::to_string(d_) + ".feather");
      item.m_dds[d_] = load_feather_data(input_file, "", {}, false, should_print);
    } else if (item.input_format == "csv") {
      auto input_file = path_join(item.input_folder, std::to_string(d_) + ".csv");
      item.m_dds[d_] = load_csv_data(input_file, "", {}, false, should_print);
    } else {
      throw std::runtime_error("not support for " + item.input_format);
    }
  }
}

std::shared_ptr<DayData> load_csv_data(const std::string& input_file, const std::string& x_pattern,
                                           const std::unordered_map<std::string, bool>& x_names, bool metaOnly, bool print) {
  auto pdd = std::make_shared<DayData>();
  auto& d = *pdd;
  if (!zerg::IsFileExisted(input_file)) {
    ZLOG_THROW("missing daily data %s", input_file.c_str());
  }

  zerg::CsvReader::read(input_file, d.id, x_pattern, x_names);
  uint64_t rows = d.id.rows;
  std::vector<OutputColumnOption>* cols = &d.id.cols;

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

  if (print) {
  	std::cout << "loaded " + input_file << std::endl;
  }
  return pdd;
}

namespace {
// Helper function to check if column should be included based on select/exclude
static bool should_include_column(const std::string& col_name, 
                         const std::unordered_map<std::string, bool>& select_map,
                         const std::unordered_map<std::string, bool>& exclude_map) {
    // If select is specified, only include columns in select map
    if (!select_map.empty()) {
        auto it = select_map.find(col_name);
        if (it == select_map.end() || !it->second) return false;
    }
    
    // If exclude is specified, exclude columns in exclude map
    if (!exclude_map.empty()) {
        auto it = exclude_map.find(col_name);
        if (it != exclude_map.end() && it->second) return false;
    }
    
    return true;
}

static std::string modify_column_name(const std::string& name, 
                              const std::string& prefix, 
                              const std::string& suffix) {
    return prefix + name + suffix;
}
}

void merge_read(zerg::InputData& m_id, const nlohmann::json& config) {
  int start_date = zerg::json_get<int>(config, "start_date");
  int end_date = zerg::json_get<int>(config, "end_date");
  check_to(end_date);
  std::string calendar_file = zerg::json_get<std::string>(config, "calendar");
  int print_every_n = zerg::json_get<int>(config, "print_every_n", -1);
  
  zerg::BizDayConfig biz_day(calendar_file);
  std::vector<int32_t> trading_dates = biz_day.bizDayRange(start_date, end_date);
  
  if (trading_dates.empty()) {
      ZLOG_THROW("No trading dates found in range %d to %d", start_date, end_date);
  }

  std::unordered_map<std::string, bool> added_feat_names;
  std::unordered_map<std::string, int> feat2idx;
  std::vector<std::string> feat_names;
  std::vector<std::vector<double>*> feats_;
  std::vector<int>* pDate{nullptr};
  std::vector<int>* pTick{nullptr};
  std::vector<int>* pUkey{nullptr};
  
  zerg::DTUMap row_index_map;
  
  bool first_entry = true;
  auto& entries = config["entry"];
  for (auto& entry : entries) {
      if (!zerg::json_get<bool>(entry, "enable", true)) continue;
      
      std::string dir = zerg::json_get<std::string>(entry, "dir");
      std::string column_prefix = zerg::json_get<std::string>(entry, "column_prefix", "");
      std::string column_suffix = zerg::json_get<std::string>(entry, "column_suffix", "");
      std::string select_str = zerg::TrimCopy(zerg::json_get<std::string>(entry, "select", ""));
      std::string exclude_str = zerg::TrimCopy(zerg::json_get<std::string>(entry, "exclude", ""));
      std::vector<std::string> select_cols, exclude_cols;
      std::unordered_map<std::string, bool> sel_cols, ignore_cols;
      if (!select_str.empty()) {
          select_cols = zerg::split(select_str, ',');
          for (auto& col : select_cols) sel_cols[col] = true;
      }
      if (!exclude_str.empty()) {
          exclude_cols = zerg::split(exclude_str, ',');
          for (auto& col : exclude_cols) ignore_cols[col] = true;
      }

      zerg::DailyDatum d_datum;
      d_datum.input_folder = dir;
      d_datum.skip_ticktime = false;
      d_datum.skip_date_check = false;
      d_datum.use_prev_date_data = false;
      d_datum.nday = trading_dates.size();
      d_datum.input_format = zerg::json_get<std::string>(entry, "data_type", "feather");
      zerg::LoadDailyDatum(d_datum, trading_dates, ignore_cols, print_every_n);
      
      bool first_date = true;
      for (int32_t date : trading_dates) {
          if (d_datum.m_dds.find(date) == d_datum.m_dds.end()) {
              ZLOG_THROW("cannot find %d for entry %s", date, d_datum.input_folder.c_str());
          }
          std::shared_ptr<zerg::DayData>& dd_ = d_datum.m_dds[date];
          size_t curr_len = dd_->x_dates->size();
          
          if (first_entry) {
              row_index_map.build_dtu_idx(*dd_->x_dates, *dd_->x_ticks, *dd_->x_ukeys, pUkey ? pUkey->size() : 0UL);
              if (first_date) {
                  // For the first date of the first entry, initialize all vectors and index map
                  pDate = m_id.new_int_vec(0);
                  pTick = m_id.new_int_vec(0);
                  pUkey = m_id.new_int_vec(0);
                  for (size_t fi = 0; fi < dd_->xNames.size(); fi++) {
                      if (should_include_column(dd_->xNames[fi], sel_cols, ignore_cols)) {
                          auto new_col_name = modify_column_name(dd_->xNames[fi], column_prefix, column_suffix);
                          if (added_feat_names[new_col_name]) {
                              ZLOG_THROW("dupe name found for %d, entry=%s", date, dir.c_str());
                          } else {
                              added_feat_names[new_col_name] = true;
                              feat2idx[new_col_name] = feats_.size();
                              feat_names.push_back(new_col_name);
                              std::vector<double>* tmp_feat = m_id.new_double_vec(0);
                              feats_.push_back(tmp_feat);
                          }
                      }
                  }
              } 
              // Append date/tick/ukey
              pDate->insert(pDate->end(), dd_->x_dates->begin(), dd_->x_dates->end());
              pTick->insert(pTick->end(), dd_->x_ticks->begin(), dd_->x_ticks->end());
              pUkey->insert(pUkey->end(), dd_->x_ukeys->begin(), dd_->x_ukeys->end());
              
              // Append feature data
              for (size_t fi = 0; fi < dd_->xNames.size(); fi++) {
                  if (should_include_column(dd_->xNames[fi], sel_cols, ignore_cols)) {
                      auto new_col_name = modify_column_name(dd_->xNames[fi], column_prefix, column_suffix);
                      auto it = feat2idx.find(new_col_name);
                      if (it != feat2idx.end()) {
                          // Append to existing feature vector
                          auto& existing_feat = *feats_[it->second];
                          existing_feat.insert(existing_feat.end(), dd_->pXs[fi]->begin(), dd_->pXs[fi]->end());
                      } else {
                          // This shouldn't happen if logic is correct
                          ZLOG_THROW("Feature %s not found in feat2idx map", new_col_name.c_str());
                      }
                  }
              }
          } else {
              if (first_date) {
                  for (size_t fi = 0; fi < dd_->xNames.size(); fi++) {
                      if (should_include_column(dd_->xNames[fi], sel_cols, ignore_cols)) {
                          auto new_col_name = modify_column_name(dd_->xNames[fi], column_prefix, column_suffix);
                          if (added_feat_names[new_col_name]) {
                              ZLOG_THROW("dupe name found for %d, entry=%s", date, dir.c_str());
                          } else {
                              added_feat_names[new_col_name] = true;
                              feat2idx[new_col_name] = feats_.size();
                              feat_names.push_back(new_col_name);
                              std::vector<double>* tmp_feat = m_id.new_double_vec(pUkey->size());
                              std::fill(tmp_feat->begin(), tmp_feat->end(), NAN);
                              feats_.push_back(tmp_feat);
                          }
                      }
                  }
              }
              
              // For each row in the current entry, find its position in the index map
              for (size_t i = 0; i < curr_len; i++) {
                  int date_val = (*dd_->x_dates)[i];
                  int tick_val = (*dd_->x_ticks)[i];
                  int ukey_val = (*dd_->x_ukeys)[i];
                  uint64_t target_index = row_index_map.find_dtu_idx(date_val, tick_val, ukey_val);
                  if (target_index == zerg::InValidDTUKey) continue;
                      
                  for (size_t fi = 0; fi < dd_->xNames.size(); fi++) {
                      if (should_include_column(dd_->xNames[fi], sel_cols, ignore_cols)) {
                          auto new_col_name = modify_column_name(dd_->xNames[fi], column_prefix, column_suffix);
                          auto feat_it = feat2idx.find(new_col_name);
                          
                          if (feat_it != feat2idx.end()) {
                              // Feature exists, update the value at the target index
                              (*feats_[feat_it->second])[target_index] = (*dd_->pXs[fi])[i];
                          } else {
                              ZLOG_THROW("Feature %s not found in feat2idx map", new_col_name.c_str());
                          }
                      }
                  }
              }
          } // end entry

          first_date = false;
      } // end date loop

      first_entry = false;
  }
  
  if (first_entry) {
      ZLOG_THROW("No data files were loaded from any entries");
  }

  m_id.rows = pUkey->size();
  m_id.cols.push_back({3, pDate, "DataDate"});
  m_id.cols.push_back({3, pTick, "ticktime"});
  m_id.cols.push_back({3, pUkey, "ukey"});
  for (size_t fi = 0; fi < feat_names.size(); fi++) {
      m_id.cols.push_back({1, feats_[fi], feat_names[fi]});
  }
  
  ZLOG("Loaded data for %zu trading dates from %d to %d", trading_dates.size(), start_date, end_date);
}
}