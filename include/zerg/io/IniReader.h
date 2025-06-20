// Read an INI file into easy-to-access name/value pairs.

// inih and INIReader are released under the New BSD license (see LICENSE.txt).
// Go to the project home page for more info:
//
// https://github.com/benhoyt/inih

#ifndef __INIREADER_H__
#define __INIREADER_H__

#include <algorithm>
#include <map>
#include <string>
#include <vector>
#include "ini.h"

// Read an INI file into easy-to-access name/value pairs. (Note that I've gone
// for simplicity here rather than speed, but it should be pretty decent.)
class INIReader {
public:
    // Construct INIReader and parse given filename. See ini.h for more info
    // about the parsing.
    explicit INIReader(const std::string& filename) { _error = ini_parse(filename.c_str(), ValueHandler, this); }

    // Return the result of ini_parse(), i.e., 0 on success, line number of
    // first error on parse error, or -1 on file open error.
    int ParseError() { return _error; }

    // Get a string value from INI file, returning default_value if not found.
    std::string Get(const std::string& section, const std::string& name, std::string default_value) {
        std::string key = MakeKey(section, name);
        return _values.count(key) ? _values[key] : default_value;
    }

    // Get a string value from INI file, returning default_value if not found.
    std::string GetOrThrow(const std::string& section, const std::string& name) {
        std::string key = MakeKey(section, name);
        if (!_values.count(key)) THROW_ZERG_EXCEPTION("cannot find key " << key);
        return _values[key];
    }

    // Get an integer (long) value from INI file, returning default_value if
    // not found or not a valid integer (decimal "1234", "-1234", or hex "0x4d2").
    long GetInteger(const std::string& section, const std::string& name, long default_value) {
        std::string valstr = Get(section, name, "");
        const char* value = valstr.c_str();
        char* end;
        // This parses "1234" (decimal) and also "0x4D2" (hex)
        long n = strtol(value, &end, 0);
        return end > value ? n : default_value;
    }

    // Get a real (floating point double) value from INI file, returning
    // default_value if not found or not a valid floating point value
    // according to strtod().
    double GetReal(const std::string& section, const std::string& name, double default_value) {
        std::string valstr = Get(section, name, "");
        const char* value = valstr.c_str();
        char* end;
        double n = strtod(value, &end);
        return end > value ? n : default_value;
    }

    // Get a boolean value from INI file, returning default_value if not found or if
    // not a valid true/false value. Valid true values are "true", "yes", "on", "1",
    // and valid false values are "false", "no", "off", "0" (not case sensitive).
    bool GetBoolean(const std::string& section, const std::string& name, bool default_value) {
        std::string valstr = Get(section, name, "");
        // Convert to lower case to make string comparisons case-insensitive
        std::transform(valstr.begin(), valstr.end(), valstr.begin(), ::tolower);
        if (valstr == "true" || valstr == "yes" || valstr == "on" || valstr == "1")
            return true;
        else if (valstr == "false" || valstr == "no" || valstr == "off" || valstr == "0")
            return false;
        else
            return default_value;
    }

    // Get an integer (long) value from INI file, returning default_value if
    // not found or not a valid integer (decimal "1234", "-1234", or hex "0x4d2").
    long GetIntegerOrThrow(const std::string& section, const std::string& name) {
        std::string valstr = GetOrThrow(section, name);
        const char* value = valstr.c_str();
        char* end;
        // This parses "1234" (decimal) and also "0x4D2" (hex)
        long n = strtol(value, &end, 0);
        if (end > value)
            return n;
        else
            THROW_ZERG_EXCEPTION("cannot read value " << name);
    }

    // Get a real (floating point double) value from INI file, returning
    // default_value if not found or not a valid floating point value
    // according to strtod().
    double GetRealOrThrow(const std::string& section, const std::string& name) {
        std::string valstr = GetOrThrow(section, name);
        const char* value = valstr.c_str();
        char* end;
        double n = strtod(value, &end);
        if (end > value)
            return n;
        else
            THROW_ZERG_EXCEPTION("cannot read value " << name);
    }

    // Get a boolean value from INI file, returning default_value if not found or if // not a valid true/false value.
    // Valid true values are "true", "yes", "on", "1", and valid false values are "false", "no", "off", "0" (not case
    // sensitive).
    bool GetBooleanOrThrow(const std::string& section, const std::string& name) {
        std::string valstr = GetOrThrow(section, name);
        // Convert to lower case to make string comparisons case-insensitive
        std::transform(valstr.begin(), valstr.end(), valstr.begin(), ::tolower);
        if (valstr == "true" || valstr == "yes" || valstr == "on" || valstr == "1")
            return true;
        else if (valstr == "false" || valstr == "no" || valstr == "off" || valstr == "0")
            return false;
        else
            THROW_ZERG_EXCEPTION("cannot read value " << name);
    }

    bool CheckSectionExist(const std::string& section) {
        return std::find(_sections.begin(), _sections.end(), section) != _sections.end();
    }

    const std::vector<std::string>& GetSections() { return _sections; }

private:
    int _error;
    std::map<std::string, std::string> _values;
    std::vector<std::string> _sections;

    static std::string MakeKey(const std::string& section, const std::string& name) {
        std::string key = section + "=" + name;
        // Convert to lower case to make section/name lookups case-insensitive
        // std::transform(key.begin(), key.end(), key.begin(), ::tolower);
        return key;
    }

    static int ValueHandler(void* user, const char* section, const char* name, const char* value) {
        INIReader* reader = static_cast<INIReader*>(user);
        std::string key = MakeKey(section, name);
        if (!reader->_values[key].empty()) reader->_values[key] += "\n";
        reader->_values[key] += value;
        reader->_sections.push_back(section);
        return 1;
    }
};

#endif
