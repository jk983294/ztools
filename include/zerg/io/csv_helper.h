#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <zerg/io/input_data.h>

namespace zerg {
using std::string;

struct CsvReader {
	char delimiter{','};
	bool has_header{true};
	size_t nrow{0};
	std::string head_line;
	std::vector<int> field_idx;
	std::vector<string> field_names; // to load fields
	std::vector<string> header_columns;
	std::vector<string> contents;
	std::vector<std::vector<string>> columns;

	std::vector<std::string> split(const std::string& str, char delimiter_);
	bool read_file(std::string path);
	bool parse_header();
	void parse();
	bool is_double_vec(const std::vector<string>& vec);
	std::vector<double> to_double_vec(const std::vector<string>& vec);
	std::vector<int> to_int_vec(const std::vector<string>& vec);
	int num_columns() const { return field_idx.size(); }
	size_t num_rows() const { return nrow; }
	std::string name(int i) const { return header_columns[field_idx[i]]; }
	const std::vector<string>& col_data(int i) const { return columns[i]; }

	static void read(std::string path_, InputData& id, const std::string& x_pattern = "", const std::unordered_map<std::string, bool>& x_names = {});
};
}