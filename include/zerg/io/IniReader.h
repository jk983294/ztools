// Read an INI file into easy-to-access name/value pairs.

// inih and INIReader are released under the New BSD license (see LICENSE.txt).
// Go to the project home page for more info:
//
// https://github.com/benhoyt/inih

#pragma once

#include <map>
#include <string>
#include <vector>

namespace zerg {
// Read an INI file into easy-to-access name/value pairs. (Note that I've gone
// for simplicity here rather than speed, but it should be pretty decent.)
class INIReader {
public:
    // Construct INIReader and parse given filename. See ini.h for more info
    // about the parsing.
    explicit INIReader(const std::string& filename);

    // Return the result of ini_parse(), i.e., 0 on success, line number of
    // first error on parse error, or -1 on file open error.
    int ParseError();

    // Get a string value from INI file, returning default_value if not found.
    std::string Get(const std::string& section, const std::string& name, std::string default_value);

    // Get a string value from INI file, returning default_value if not found.
    std::string GetOrThrow(const std::string& section, const std::string& name);

    // Get an integer (long) value from INI file, returning default_value if
    // not found or not a valid integer (decimal "1234", "-1234", or hex "0x4d2").
    long GetInteger(const std::string& section, const std::string& name, long default_value);

    // Get a real (floating point double) value from INI file, returning
    // default_value if not found or not a valid floating point value
    // according to strtod().
    double GetReal(const std::string& section, const std::string& name, double default_value);

    // Get a boolean value from INI file, returning default_value if not found or if
    // not a valid true/false value. Valid true values are "true", "yes", "on", "1",
    // and valid false values are "false", "no", "off", "0" (not case sensitive).
    bool GetBoolean(const std::string& section, const std::string& name, bool default_value);

    // Get an integer (long) value from INI file, returning default_value if
    // not found or not a valid integer (decimal "1234", "-1234", or hex "0x4d2").
    long GetIntegerOrThrow(const std::string& section, const std::string& name);

    // Get a real (floating point double) value from INI file, returning
    // default_value if not found or not a valid floating point value
    // according to strtod().
    double GetRealOrThrow(const std::string& section, const std::string& name);

    // Get a boolean value from INI file, returning default_value if not found or if // not a valid true/false value.
    // Valid true values are "true", "yes", "on", "1", and valid false values are "false", "no", "off", "0" (not case
    // sensitive).
    bool GetBooleanOrThrow(const std::string& section, const std::string& name);

    bool CheckSectionExist(const std::string& section);

    const std::vector<std::string>& GetSections();

private:
    int _error;
    std::map<std::string, std::string> _values;
    std::vector<std::string> _sections;

    static std::string MakeKey(const std::string& section, const std::string& name);
    static int ValueHandler(void* user, const char* section, const char* name, const char* value);
};
}