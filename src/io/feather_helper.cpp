#include <arrow/api.h>
#include <arrow/filesystem/api.h>
#include <arrow/io/file.h>
#include <arrow/ipc/api.h>
#include <arrow/ipc/writer.h>
#include <arrow/result.h>
#include <arrow/type.h>
#include <arrow/util/io_util.h>
#include <zerg/io/input_data.h>
#include <zerg/io/feather_helper.h>
#include <zerg/io/file.h>
#include <regex>
#include <cstdio>
#include <iostream>

namespace zerg {
namespace {
inline void get_data_by_arrow_type_helper(double val, int& out) { out = static_cast<int>(std::lround(val)); }
inline void get_data_by_arrow_type_helper(int val, int& out) { out = val; }
inline void get_data_by_arrow_type_helper(int64_t val, int& out) { out = val; }
inline void get_data_by_arrow_type_helper(double val, double& out) { out = val; }
inline void get_data_by_arrow_type_helper(float val, double& out) { out = val; }

template <typename TIn, typename TOut>
inline void get_array_data(std::vector<TOut>& vec, std::shared_ptr<arrow::Table>& Tbl, int idx, int64_t rows) {
    vec.resize(rows);
    auto pChArray = Tbl->column(idx);
    int NChunks = pChArray->num_chunks();
    int i = 0;
    for (int n = 0; n < NChunks; n++) {
        auto pArray = pChArray->chunk(n);
        int64_t ArrayRows = pArray->length();
        auto pTypedArray = std::dynamic_pointer_cast<TIn>(pArray);
        const auto* pData = pTypedArray->raw_values();
        for (int64_t j = 0; j < ArrayRows; j++) {
            TOut tmp;
            get_data_by_arrow_type_helper(pData[j], tmp);
            vec[i++] = tmp;
        }
    }
}

inline void get_array_data_bool(std::vector<bool>& vec, std::shared_ptr<arrow::Table>& Tbl, int idx, int64_t rows) {
    vec.resize(rows);
    auto pChArray = Tbl->column(idx);
    int NChunks = pChArray->num_chunks();
    int i = 0;
    for (int n = 0; n < NChunks; n++) {
        std::shared_ptr<arrow::Array> pArray = pChArray->chunk(n);
        int64_t ArrayRows = pArray->length();
        auto pTypedArray = std::dynamic_pointer_cast<arrow::BooleanArray>(pArray);
        for (int64_t j = 0; j < ArrayRows; j++) {
            vec[i++] = pTypedArray->Value(j);
        }
    }
}

inline std::shared_ptr<arrow::Table> read_feather(const std::string& path_) {
  auto target_file = path_;
  auto inputv = arrow::io::ReadableFile::Open(target_file);
  if (!inputv.ok()) {
    std::string errmsg = "open file " + target_file + " error";
    // logger->trace(errmsg);
    // logger->flush();
    throw std::runtime_error(errmsg);
  }
  const auto& input = inputv.ValueOrDie();
  auto st = arrow::ipc::RecordBatchFileReader::Open(input);
  if (!st.ok()) {
    // logger->trace("[LoadUkey] open st error");
    throw std::runtime_error("invalid st");
  }
  auto reader = st.ValueOrDie();
  int num_batches = reader->num_record_batches();
  std::vector<std::shared_ptr<arrow::RecordBatch>> batches(num_batches);
  for (int i = 0; i < num_batches; i++) {
    batches[i] = reader->ReadRecordBatch(i).ValueOrDie();
  }

  auto table_opt = arrow::Table::FromRecordBatches(batches);
  if (!table_opt.ok()) {
    std::cout << "invalid arrow table" << std::endl;
  }
  auto table_val = table_opt.ValueOrDie()->CombineChunks();
  if (!table_val.ok()) {
    std::cout << "invalid combine table" << std::endl;
  }

  return table_val.ValueOrDie();
}
}  // namespace


void FeatherReader::read(std::string path_, InputData& id, const std::string& x_pattern,
    const std::unordered_map<std::string, bool>& x_names) {
    path_ = zerg::FileExpandUser(path_);
    auto table1 = read_feather(path_);
    auto schema_ = table1->schema();
    auto col_cnt = table1->num_columns();
    id.rows = table1->num_rows();
    std::regex x_regex(x_pattern);

    for (int i = 0; i < col_cnt; ++i) {
        auto f_ = schema_->field(i);
        auto type_ = f_->type()->name();
        auto name = f_->name();

        bool all_empty = x_pattern.empty() && x_names.empty();
        bool found_name = x_names.find(name) != x_names.end();
        bool found_pattern = (!x_pattern.empty() && std::regex_search(name, x_regex));

        if (!found_name && !found_pattern && !all_empty) {
            continue;
        }

        if (type_ == "int32") {
            auto* pVec = id.new_int_vec(0);
            get_array_data<arrow::Int32Array, int>(*pVec, table1, i, id.rows);
            id.cols.push_back({3, pVec, f_->name()});
        } else if (type_ == "double") {
            auto* pVec = id.new_double_vec(0);
            get_array_data<arrow::DoubleArray, double>(*pVec, table1, i, id.rows);
            id.cols.push_back({1, pVec, f_->name()});
        } else if (type_ == "float") {
            auto* pVec = id.new_double_vec(0);
            get_array_data<arrow::FloatArray, double>(*pVec, table1, i, id.rows);
            id.cols.push_back({1, pVec, f_->name()});
        } else if (type_ == "bool") {
            auto* pVec = id.new_bool_vec(0);
            get_array_data_bool(*pVec, table1, i, id.rows);
            id.cols.push_back({5, pVec, f_->name()});
        } else {
            // printf("unknown %s,%s\n", f_->name().c_str(), type_.c_str());
        }
    }
}

bool write_feather(std::string path_, size_t nrow, const std::vector<OutputColumnOption>& cols, bool use_float32) {
    path_ = zerg::FileExpandUser(path_);
    InputData id;
    std::vector<std::shared_ptr<arrow::Array>> arrays;
    arrow::SchemaBuilder sb_;
    std::vector<std::shared_ptr<arrow::NumericBuilder<arrow::DoubleType>>> d_builders;
    std::vector<std::shared_ptr<arrow::NumericBuilder<arrow::FloatType>>> f_builders;
    std::vector<std::shared_ptr<arrow::NumericBuilder<arrow::Int32Type>>> i_builders;
    std::vector<std::shared_ptr<arrow::NumericBuilder<arrow::UInt64Type>>> ui_builders;
    std::vector<std::shared_ptr<arrow::StringBuilder>> str_builders;
    for (size_t i = 0; i < cols.size(); ++i) {
        auto& option = cols[i];
        if (option.type == 1) {
            if (use_float32) {
                auto tmp_builder = std::make_shared<arrow::NumericBuilder<arrow::FloatType>>();
                std::vector<float>* f_vec = id.new_float_vec(nrow);
                float* pf = f_vec->data();
                auto* src_vec = reinterpret_cast<double*>(option.data);
                for (size_t j = 0; j < nrow; ++j) {
                    pf[j] = src_vec[j];
                }
                std::ignore = tmp_builder->AppendValues(pf, nrow);
                std::shared_ptr<arrow::Array> tmp_a;
                std::ignore = tmp_builder->Finish(&tmp_a);
                tmp_builder->Reset();
                f_builders.push_back(tmp_builder);
                arrays.push_back(tmp_a);
                std::ignore = sb_.AddField(arrow::field(option.name, arrow::float32()));
            } else {
                auto tmp_builder = std::make_shared<arrow::NumericBuilder<arrow::DoubleType>>();
                std::ignore = tmp_builder->AppendValues(reinterpret_cast<double*>(option.data), nrow);
                std::shared_ptr<arrow::Array> tmp_a;
                std::ignore = tmp_builder->Finish(&tmp_a);
                tmp_builder->Reset();
                d_builders.push_back(tmp_builder);
                arrays.push_back(tmp_a);
                std::ignore = sb_.AddField(arrow::field(option.name, arrow::float64()));
            }
        } else if (option.type == 2) {
            auto tmp_builder = std::make_shared<arrow::NumericBuilder<arrow::FloatType>>();
            std::ignore = tmp_builder->AppendValues(reinterpret_cast<float*>(option.data), nrow);
            std::shared_ptr<arrow::Array> tmp_a;
            std::ignore = tmp_builder->Finish(&tmp_a);
            tmp_builder->Reset();
            f_builders.push_back(tmp_builder);
            arrays.push_back(tmp_a);
            std::ignore = sb_.AddField(arrow::field(option.name, arrow::float32()));
        } else if (option.type == 3) {
            auto tmp_builder = std::make_shared<arrow::NumericBuilder<arrow::Int32Type>>();
            std::ignore = tmp_builder->AppendValues(reinterpret_cast<int*>(option.data), nrow);
            std::shared_ptr<arrow::Array> tmp_a;
            std::ignore = tmp_builder->Finish(&tmp_a);
            tmp_builder->Reset();
            i_builders.push_back(tmp_builder);
            arrays.push_back(tmp_a);
            std::ignore = sb_.AddField(arrow::field(option.name, arrow::int32()));
        } else if (option.type == 4) {
            auto tmp_builder = std::make_shared<arrow::StringBuilder>();
            std::vector<std::string>& sVec = *reinterpret_cast<std::vector<std::string>*>(option.data);
            std::ignore = tmp_builder->AppendValues(sVec);
            std::shared_ptr<arrow::Array> tmp_a;
            std::ignore = tmp_builder->Finish(&tmp_a);
            tmp_builder->Reset();
            str_builders.push_back(tmp_builder);
            arrays.push_back(tmp_a);
            std::ignore = sb_.AddField(arrow::field(option.name, arrow::utf8()));
        } else if (option.type == 6) {
            auto tmp_builder = std::make_shared<arrow::NumericBuilder<arrow::UInt64Type>>();
            std::ignore = tmp_builder->AppendValues(reinterpret_cast<uint64_t*>(option.data), nrow);
            std::shared_ptr<arrow::Array> tmp_a;
            std::ignore = tmp_builder->Finish(&tmp_a);
            tmp_builder->Reset();
            ui_builders.push_back(tmp_builder);
            arrays.push_back(tmp_a);
            std::ignore = sb_.AddField(arrow::field(option.name, arrow::uint64()));
        } else {
            throw std::runtime_error("write_feather un support type");
        }
    }

    auto schema = sb_.Finish().ValueOrDie();
    std::shared_ptr<arrow::Table> table = arrow::Table::Make(schema, arrays);

    std::shared_ptr<arrow::fs::FileSystem> fs = std::make_shared<arrow::fs::LocalFileSystem>();
    auto outfile_tmp = path_ + ".tmp";
    auto output = fs->OpenOutputStream(outfile_tmp).ValueOrDie();
    auto writer = arrow::ipc::MakeFileWriter(output.get(), table->schema()).ValueOrDie();
    arrow::Status status_ = writer->WriteTable(*table);
    if (!status_.ok()) {
        printf("write table %s failed, error=%s\n", path_.c_str(), status_.message().c_str());
        return false;
    }
    status_ = writer->Close();
    if (!status_.ok()) {
        printf("writer close %s failed, error=%s\n", path_.c_str(), status_.message().c_str());
        return false;
    } else {
        std::rename(outfile_tmp.c_str(), path_.c_str());
        printf("write %s success\n", path_.c_str());
        return true;
    }
}

bool write_feather(std::string path_, size_t nrow,
    const std::vector<std::string>& int_col_names, const std::vector<std::string>& double_col_names,
    const std::vector<const int*>& int_cols, const std::vector<const double*>& double_cols, bool use_float32) {
    if (int_col_names.size() != int_cols.size()) {
        throw std::runtime_error("expect int cols size equal!");
    }
    if (double_col_names.size() != double_cols.size()) {
        throw std::runtime_error("expect double cols size equal!");
    }
    InputData id;
    std::vector<OutputColumnOption> options;
    for (size_t i = 0; i < int_col_names.size(); i++) {
        options.push_back({3, (void*)int_cols[i], int_col_names[i]});
    }

    for (size_t i = 0; i < double_col_names.size(); i++) {
        options.push_back({1, (void*)double_cols[i], double_col_names[i]});
    }
    return write_feather(path_, nrow, options, use_float32);
}
bool write_feather(std::string path_, const std::vector<int>& ints, const std::vector<double>& doubles, bool use_float32) {
    if (ints.size() != doubles.size()) {
        throw std::runtime_error("expect int/doubles col size equal!");
    }
    return write_feather(path_, ints.size(), {"key"}, {"value"}, {ints.data()}, {doubles.data()}, use_float32);
}
bool write_feather(std::string path_, const std::vector<int>& vs, bool use_float32) {
    return write_feather(path_, vs.size(), {"value"}, {}, {vs.data()}, {}, use_float32);
}
bool write_feather(std::string path_, const std::vector<double>& vs, bool use_float32) {
    return write_feather(path_, vs.size(), {}, {"value"}, {}, {vs.data()}, use_float32);
}
}