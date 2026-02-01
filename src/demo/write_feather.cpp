#include <zerg/io/feather_helper.h>

using namespace zerg;

int main() {
    std::string path_ = "/tmp/data2.feather";
    size_t nrOfRows = 1000;

    std::vector<double> d_vec(nrOfRows);
    std::vector<float> f_vec(nrOfRows);
    std::vector<int> i_vec(nrOfRows);
    std::vector<std::string> s_vec(nrOfRows);
    std::vector<bool> b_vec(nrOfRows);
    for (size_t i = 0; i < nrOfRows; ++i) {
        d_vec[i] = i;
        f_vec[i] = i;
        i_vec[i] = i;
        s_vec[i] = std::to_string(i);
        b_vec[i] = i % 2 == 0;
    }

    std::vector<OutputColumnOption> options;
    options.push_back({1, d_vec.data(), "my_double"});
    options.push_back({2, f_vec.data(), "my_float"});
    options.push_back({3, i_vec.data(), "my_int"});
    options.push_back({4, &s_vec, "my_str"});
    options.push_back({5, &b_vec, "my_bool"});

    write_feather(path_, nrOfRows, options, true);
    return 0;
}
