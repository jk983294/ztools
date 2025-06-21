#include <string>
#include "catch.hpp"
#include "csv.h"

using namespace std;

TEST_CASE("test file case 1", "[csv]") {
    io::CSVReader<4> infile("/opt/version/latest/test/test.csv");
    infile.read_header(io::ignore_extra_column, "name", "int", "float", "double");
    std::string name;
    int a;
    float b;
    double c;
    while (infile.read_row(name, a, b, c)) {
        REQUIRE(name == "kun");
        REQUIRE(a == 1);
        REQUIRE(b == 1.0);
        REQUIRE(c == 1.0);
    }
}

TEST_CASE("no trim, separate by '|'", "[csv]") {
    io::CSVReader<4, io::trim_chars<>, io::no_quote_escape<'|'> > infile("/opt/version/latest/test/test2.csv");
    infile.read_header(io::ignore_extra_column, "name", "int", "float", "double");
    std::string name;
    int a;
    float b;
    double c;
    int sum = 0;
    while (infile.read_row(name, a, b, c)) {
        REQUIRE(name == "kun");
        REQUIRE(a == 1);
        REQUIRE(b == 1.0);
        REQUIRE(c == 1.0);
        ++sum;
    }
    REQUIRE(sum == 1);
}

TEST_CASE("no trim, double quote, separate by '|', ignore empty line", "[csv]") {
    io::CSVReader<4, io::trim_chars<>, io::double_quote_escape<'|', '\"'>, io::throw_on_overflow,
                  io::empty_line_comment>
        infile("/opt/version/latest/test/test3.csv");
    infile.read_header(io::ignore_extra_column, "name", "int", "float", "double");
    std::string name;
    int a;
    float b;
    double c;
    int sum = 0;
    while (infile.read_row(name, a, b, c)) {
        sum += a;
        REQUIRE(name == "  kun  ");
        REQUIRE(a == 1);
        REQUIRE(b == 1.0);
        REQUIRE(c == 1.0);
    }
    REQUIRE(sum == 1);
    REQUIRE(infile.get_file_line() == 3);
}

TEST_CASE("column rotated", "[csv]") {
    io::CSVReader<4> infile("/opt/version/latest/test/test.csv");
    infile.read_header(io::ignore_extra_column, "name", "double", "int", "float");
    std::string name;
    int a;
    float b;
    double c;
    while (infile.read_row(name, b, a, c)) {
        REQUIRE(name == "kun");
        REQUIRE(a == 1);
        REQUIRE(b == 1.0);
        REQUIRE(c == 1.0);
    }
    REQUIRE(infile.get_file_line() == 2);
}

TEST_CASE("no header", "[csv]") {
    io::CSVReader<5> infile("/opt/version/latest/test/test4.csv");
    infile.set_header("name", "int", "string", "float", "double");
    std::string name, str;
    int a;
    float b;
    double c;
    while (infile.read_row(name, a, str, b, c)) {
        REQUIRE(name == "kun");
        REQUIRE(a == 1);
        REQUIRE(str == "kun");
        REQUIRE(b == 1.0);
        REQUIRE(c == 1.0);
    }
}
