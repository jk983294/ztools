#include <zerg/io/feather_helper.h>

using namespace zerg;

static void read_id(InputData& reader) {
  printf("rows=%zu\n", reader.rows);
  for (auto& col : reader.cols) {
    if (col.type == 1) {
      auto& vec = *reinterpret_cast<std::vector<double>*>(col.data);
      printf("col %s type=%d data=%f,%f\n", col.name.c_str(), col.type, vec.front(), vec.back());
    } else if (col.type == 3) {
      auto& vec = *reinterpret_cast<std::vector<int>*>(col.data);
      printf("col %s type=%d data=%d,%d\n", col.name.c_str(), col.type, vec.front(), vec.back());
    } else if (col.type == 4) {
      auto& vec = *reinterpret_cast<std::vector<std::string>*>(col.data);
      printf("col %s type=%d data=%s,%s\n", col.name.c_str(), col.type, vec.front().c_str(), vec.back().c_str());
    } else if (col.type == 5) {
      auto& vec = *reinterpret_cast<std::vector<bool>*>(col.data);
      printf("col %s type=%d data=%d,%d\n", col.name.c_str(), col.type, (int)vec.front(), (int)vec.back());
    }
  }
}

void read_dd() {
  std::string path_ = "/data/cneq/eod/20230630.feather";
  std::shared_ptr<DayData> dd = load_feather_data(path_);
  read_id(dd->id);
}

int main() {
  // std::string path_ = "/tmp/data2.feather";
  // InputData reader;
  // FeatherReader::read(path_, reader);
  // read_id(reader);

  read_dd();
  return 0;
}
