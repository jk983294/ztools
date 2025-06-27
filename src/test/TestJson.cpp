#include <string>
#include "catch.hpp"
#include <zerg/io/json_util.h>

using namespace std;
using namespace zerg;

TEST_CASE("test json", "[json]") {
    std::string json_str = R"({
        "name": "Alice",
        "age": 30,
        "is_student": false,
        "grades": [90.5, 85.0, 92.3]
    })";
    nlohmann::json j = nlohmann::json::parse(json_str);
    REQUIRE(json_get<std::string>(j, "name") == "Alice");
    REQUIRE(json_get<std::string>(j, "name", "dft_name") == "Alice");
    REQUIRE(json_get<int>(j, "age", 0) == 30);
    REQUIRE(json_get<bool>(j, "is_student", false) == false);
    std::vector<double> grades;
    json_get(j, "grades", grades);
    REQUIRE(grades.size() == 3);
}

