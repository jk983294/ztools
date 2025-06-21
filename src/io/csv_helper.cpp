#include <zerg/io/csv_helper.h>
#include <zerg/io/file.h>
#include <algorithm>
#include <regex>

namespace zerg {
std::vector<std::string> CsvReader::split(const std::string& str, char delimiter_) {
  std::vector<std::string> result;
  size_t start = 0;
  size_t end = str.find(delimiter_);
  while (end != std::string::npos) {
    result.push_back(str.substr(start, end - start));
    start = end + 1;
    end = str.find(delimiter_, start);
  }
  result.push_back(str.substr(start, end));
  return result;
}

bool CsvReader::read_file(std::string path) {
  std::ifstream ifs(path, std::ifstream::in);
  if (ifs.is_open()) {
    string s;
    while (getline(ifs, s)) {
      if (!s.empty()) contents.push_back(s);
    }
    ifs.close();
    return true;
  } else {
    return false;
  }
}
bool CsvReader::parse_header() {
  if (!has_header) {
    throw std::runtime_error("expect header!");
  }
  head_line = contents.front();
  contents.erase(contents.begin());  // remove head line
  header_columns = split(head_line, delimiter);
  if (!field_names.empty()) {
    for (const auto& name : field_names) {
      auto itr = std::find(header_columns.begin(), header_columns.end(), name);
      if (itr == header_columns.end()) {
        printf("column %s not found!", name.c_str());
        return false;
      }
      field_idx.push_back(itr - header_columns.begin());
    }
  } else {
    for (size_t i = 0; i < header_columns.size(); i++) {
      field_idx.push_back(i);
    }
  }
  return true;
}

void CsvReader::read(std::string path_, InputData& id, const std::string& x_pattern,
  const std::unordered_map<std::string, bool>& x_names) {
  path_ = FileExpandUser(path_);
  CsvReader reader;
  if(not reader.read_file(path_)) {
    throw std::runtime_error("read_file failed " + path_);
  }

  reader.parse();

  auto col_cnt = reader.num_columns();
  id.rows = reader.num_rows();
  std::regex x_regex(x_pattern);

  for (int i = 0; i < col_cnt; ++i) {
      auto name = reader.name(i);

      bool all_empty = x_pattern.empty() && x_names.empty();
      bool found_name = x_names.find(name) != x_names.end();
      bool found_pattern = (!x_pattern.empty() && std::regex_search(name, x_regex));

      if (!found_name && !found_pattern && !all_empty) {
        continue;
      }

      const auto& col_d = reader.col_data(i);

      if (name == "DataDate" || name == "ukey" || name == "ticktime") {
        auto* pVec = id.new_int_vec(0);
        *pVec = reader.to_int_vec(col_d);
        id.cols.push_back({3, pVec, name});
      } else if (reader.is_double_vec(col_d)) {
          auto* pVec = id.new_double_vec(0);
          *pVec = reader.to_double_vec(col_d);
          id.cols.push_back({1, pVec, name});
      } else {
          // printf("unknown %s,%s\n", f_->name().c_str(), type_.c_str());
      }
  }
}

void CsvReader::parse() {
  if (!parse_header()) {
    throw std::runtime_error("CsvReader parse_header failed!");
  }

  nrow = contents.size();
  columns.resize(field_idx.size(), std::vector<string>(nrow));
  for (size_t i = 0; i < nrow; i++) {
    auto lets = split(contents[i], delimiter);
    for (size_t j = 0; j < field_idx.size(); j++) {
      columns[j][i] = lets[field_idx[j]];
    }
  }
}

bool CsvReader::is_double_vec(const std::vector<string>& vec) {
  for (size_t i = 0; i < vec.size(); i++) {
    if (!vec[i].empty() && std::isalpha(vec[i].front())) {
      return false;
    }
  }
  return true;
}

std::vector<double> CsvReader::to_double_vec(const std::vector<string>& vec) {
  std::vector<double> ret(vec.size());
  for (size_t i = 0; i < vec.size(); i++) {
    if (vec[i].empty()) ret[i] = NAN;
    else ret[i] = std::stod(vec[i]);
  }
  return ret;
}
std::vector<int> CsvReader::to_int_vec(const std::vector<string>& vec) {
  std::vector<int> ret(vec.size());
  for (size_t i = 0; i < vec.size(); i++) {
    ret[i] = std::stoi(vec[i]);
  }
  return ret;
}

}