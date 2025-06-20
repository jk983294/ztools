#pragma once

#include <stdio.h>
#include <array>
#include <cstring>
#include <deque>
#include <list>
#include <map>
#include <memory>
#include <stack>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>


#define ARCHIVER_READING 1    // archiver is read-only
#define ARCHIVER_APPENDING 2  // archiver is write-only

namespace zerg {
using namespace std;
/**
 * struct MYCLASS {
 *      int var1, var2, var3, var4;
 *      CHECKPOINT_SUPPORT(var1 * var2 * var3)
 * };
 *
 * if you just want to save the entire class, and the class is trivial constructable:
 * 1. put CHECKPOINT_SUPPORT_STRUCT(MYCLASS) in your .cpp code
 * 2. put CHECKPOINT_SUPPORT_STRUCT_DECLARE(MYCLASS) in your .h code AFTER (outside) the class definition.
 *
 * if your class has virtual functions
 * use CHECKPOINT_SUPPORT_VIRTUAL(var1) so that the checkpoint function is virtual and can be overloaded
 *
 * if base class has checkpoint support code, then add CHECKPOINT_SUPPORT_DERIVED(BASE_CLASS_NAME, var1 * var2)
 *
 * If customize checkpoint code, define below functions in your class, remove virtual if no need inheritance
 * (virtual) void checkpoint_save(ArchiverWriter & writer) const;
 * (virtual) void checkpoint_load(ArchiverReader & reader);
 * (virtual) void checkpoint_bytes(ArchiverSizer & sizer) const;
 *
 * CHECKPOINT_SUPPORT_FRIEND(TClass) used for non public function definition
 */

// return generated file size in bytes. return 0 when not saved. return -1 when error
template <typename T>
int save_checkpoint(const T &var, const std::string &filename);
// return total size in bytes. return -1 when error
template <typename T>
int load_checkpoint(T &var, const std::string &filename);

class ArchiverReader {
public:
    ArchiverReader() = default;

    ~ArchiverReader() { reset(); }

    void read_data(void *data, size_t bytes) {
        memcpy(data, m_pointer, bytes);
        m_pointer += bytes;
    }
    // read all data from file. return bytes loaded.
    int read_from_file(const std::string &filename);
    // read all data from memory. return bytes loaded.
    int read_from_memory(const void *buffer, size_t bytes);

    size_t getTotalSize() const { return data_size; }
    size_t getUsedSize() const {
        if (m_buffer == nullptr) return 0;
        return m_pointer - m_buffer;
    }
    char *getDataPointer() const { return m_buffer; }

    void reset();  // reset and release all resources

    void alloc_mem(size_t bytes);  // prepare memory. delete previous content if exists.

private:
    char *m_buffer{nullptr}, *m_pointer{nullptr};
    size_t data_size{0};
};

class ArchiverWriter {
public:
    ArchiverWriter() = default;
    ~ArchiverWriter() { reset(); }

    // append data, and move pointer. need to alloc_mem first, will not check boundary
    void append_data(const void *data, size_t bytes) {
        memcpy(m_pointer, data, bytes);
        m_pointer += bytes;
    }
    // save all data to file. return false when error
    bool write_to_file(const std::string &filename) const;

    size_t getTotalSize() const { return data_size; }
    size_t getUsedSize() const {
        if (m_buffer == nullptr) return 0;
        return m_pointer - m_buffer;
    }
    char *getDataPointer() const { return m_buffer; }

    void reset();

    void alloc_mem(size_t bytes);  // prepare memory. delete previous content if exists.

private:
    char *m_buffer{nullptr}, *m_pointer{nullptr};
    size_t data_size{0};
};

// this class is used to help calculating size of objects
class ArchiverSizer {
public:
    ArchiverSizer() { m_size = 0; }
    size_t getTotalSize() const { return m_size; }
    void addSize(int s) { m_size += s; }
    void clear() { m_size = 0; }

private:
    size_t m_size;
};

inline int ArchiverReader::read_from_file(const std::string &filename) {
    reset();
    FILE *fp = fopen(filename.c_str(), "rb");
    if (fp == nullptr) return 0;
    fseek(fp, 0, SEEK_END);
    auto s = ftell(fp);
    ::rewind(fp);
    if (s > 0) {
        alloc_mem((size_t)s);
        std::ignore = fread(m_buffer, (size_t)s, 1, fp);
    }
    fclose(fp);
    return (int)s;
}

inline int ArchiverReader::read_from_memory(const void *buffer, size_t bytes) {
    reset();
    if (bytes > 0) {
        alloc_mem(bytes);
        memcpy(m_buffer, buffer, bytes);
    }
    return (int)bytes;
}

inline void ArchiverReader::reset() {
    delete[] m_buffer;
    m_buffer = nullptr;
    m_pointer = nullptr;
    data_size = 0;
}

inline void ArchiverReader::alloc_mem(size_t bytes) {
    reset();
    m_buffer = new char[bytes];
    m_pointer = m_buffer;
    data_size = bytes;
}

inline void ArchiverWriter::alloc_mem(size_t bytes) {
    reset();
    m_buffer = new char[bytes];
    m_pointer = m_buffer;
    data_size = bytes;
}

inline void ArchiverWriter::reset() {
    delete[] m_buffer;
    m_buffer = nullptr;
    m_pointer = nullptr;
    data_size = 0;
}

inline bool ArchiverWriter::write_to_file(const std::string &filename) const {
    if (data_size == 0 || m_buffer == nullptr) return true;
    FILE *fp = fopen(filename.c_str(), "wb");
    if (fp == nullptr) {
        printf("creating %s error!", filename.c_str());
        return false;
    }
    fwrite(m_buffer, data_size, 1, fp);
    fclose(fp);
    return true;
}

// return generated file size in bytes. return 0 when not saved. return -1 when error
template <typename T>
int save_checkpoint(const T &var, const std::string &filename) {
    ArchiverWriter writer;
    ArchiverSizer sizer;
    sizer *var;
    size_t total_size = sizer.getTotalSize();
    if (total_size == 0) return 0;
    writer.alloc_mem(total_size);
    writer *var;
    if (!writer.write_to_file(filename)) return -1;
    size_t ret = writer.getUsedSize();
    if (ret != sizer.getTotalSize()) {
        printf("warning: size written is not the same with size estimated when saving checkpoint\n");
    }
    return (int)ret;
}

// return total size in bytes. return -1 when error
template <typename T>
int load_checkpoint(T &var, const std::string &filename) {
    ArchiverReader reader;
    if (reader.read_from_file(filename) <= 0) return -1;
    reader *var;
    size_t ret = reader.getUsedSize();
    if (ret != reader.getTotalSize()) {
        printf("warning: size read is not the same with size estimated when loading checkpoint\n");
    }
    return (int)ret;
}

#define CHECKPOINT_SUPPORT_FRIEND(TClass)                                        \
    inline ArchiverWriter &operator*(ArchiverWriter &writer, const TClass &var); \
    inline ArchiverReader &operator*(ArchiverReader &reader, TClass &var);       \
    inline ArchiverSizer &operator*(ArchiverSizer &sizer, const TClass &var);

#define CHECKPOINT_SUPPORT(var_list)                                                \
    inline void checkpoint_save(ArchiverWriter &writer) const { writer *var_list; } \
    inline void checkpoint_load(ArchiverReader &reader) { reader *var_list; }       \
    inline void checkpoint_bytes(ArchiverSizer &sizer) const { sizer *var_list; }

#define CHECKPOINT_SUPPORT_VIRTUAL(var_list)                                                \
    inline virtual void checkpoint_save(ArchiverWriter &writer) const { writer *var_list; } \
    inline virtual void checkpoint_load(ArchiverReader &reader) { reader *var_list; }       \
    inline virtual void checkpoint_bytes(ArchiverSizer &sizer) const { sizer *var_list; }

#define CHECKPOINT_SUPPORT_DERIVED(base_class_name, var_list)           \
    inline virtual void checkpoint_save(ArchiverWriter &writer) const { \
        base_class_name::checkpoint_save(writer);                       \
        writer *var_list;                                               \
    }                                                                   \
    inline virtual void checkpoint_load(ArchiverReader &reader) {       \
        base_class_name::checkpoint_load(reader);                       \
        reader *var_list;                                               \
    }                                                                   \
    inline virtual void checkpoint_bytes(ArchiverSizer &sizer) const {  \
        base_class_name::checkpoint_bytes(sizer);                       \
        sizer *var_list;                                                \
    }

#define CHECKPOINT_SUPPORT_STRUCT(type)                                                      \
    inline ArchiverWriter &operator*(ArchiverWriter &writer, const type &arg) {              \
        writer.append_data(&arg, sizeof(type));                                              \
        return writer;                                                                       \
    }                                                                                        \
    inline ArchiverReader &operator*(ArchiverReader &reader, type &arg) {                    \
        reader.read_data(&arg, sizeof(type));                                                \
        return reader;                                                                       \
    }                                                                                        \
    inline ArchiverSizer &operator*(ArchiverSizer &sizer, const type &arg) {                 \
        sizer.addSize(sizeof(arg));                                                          \
        return sizer;                                                                        \
    }                                                                                        \
    inline ArchiverWriter &operator*(ArchiverWriter &writer, const std::vector<type> &arg) { \
        size_t s = arg.size();                                                               \
        writer *s;                                                                           \
        writer.append_data(&arg[0], sizeof(type) * s);                                       \
        return writer;                                                                       \
    }                                                                                        \
    inline ArchiverReader &operator*(ArchiverReader &reader, std::vector<type> &arg) {       \
        size_t s;                                                                            \
        reader *s;                                                                           \
        arg.resize(s);                                                                       \
        reader.read_data(&arg[0], sizeof(type) * s);                                         \
        return reader;                                                                       \
    }                                                                                        \
    inline ArchiverSizer &operator*(ArchiverSizer &sizer, const std::vector<type> &arg) {    \
        sizer.addSize(sizeof(size_t) + arg.size() * sizeof(type));                           \
        return sizer;                                                                        \
    }

#define CHECKPOINT_SUPPORT_STRUCT_DECLARE(type)                                      \
    ArchiverWriter &operator*(ArchiverWriter &writer, const type &arg);              \
    ArchiverReader &operator*(ArchiverReader &reader, type &arg);                    \
    ArchiverSizer &operator*(ArchiverSizer &sizer, const type &arg);                 \
    ArchiverWriter &operator*(ArchiverWriter &writer, const std::vector<type> &arg); \
    ArchiverReader &operator*(ArchiverReader &reader, std::vector<type> &arg);       \
    ArchiverSizer &operator*(ArchiverSizer &sizer, const std::vector<type> &arg);

// define * for each basic data type
CHECKPOINT_SUPPORT_STRUCT_DECLARE(char)
CHECKPOINT_SUPPORT_STRUCT_DECLARE(unsigned char)
CHECKPOINT_SUPPORT_STRUCT_DECLARE(short)
CHECKPOINT_SUPPORT_STRUCT_DECLARE(unsigned short)
CHECKPOINT_SUPPORT_STRUCT_DECLARE(unsigned int)
CHECKPOINT_SUPPORT_STRUCT_DECLARE(unsigned long long)
CHECKPOINT_SUPPORT_STRUCT_DECLARE(long long)
CHECKPOINT_SUPPORT_STRUCT_DECLARE(float)
CHECKPOINT_SUPPORT_STRUCT_DECLARE(double)
CHECKPOINT_SUPPORT_STRUCT_DECLARE(size_t)
CHECKPOINT_SUPPORT_STRUCT_DECLARE(int64_t)
// CHECKPOINT_SUPPORT_STRUCT_DECLARE(int)  // below int example
inline ArchiverWriter &operator*(ArchiverWriter &writer, const int &arg);
inline ArchiverReader &operator*(ArchiverReader &reader, int &arg);
inline ArchiverSizer &operator*(ArchiverSizer &sizer, const int &arg);
inline ArchiverWriter &operator*(ArchiverWriter &writer, const std::vector<int> &arg);
inline ArchiverReader &operator*(ArchiverReader &reader, std::vector<int> &arg);
inline ArchiverSizer &operator*(ArchiverSizer &sizer, const std::vector<int> &arg);

// bool is special as vector<bool> is optimized using bit, not byte
inline ArchiverWriter &operator*(ArchiverWriter &writer, const bool &arg);
inline ArchiverReader &operator*(ArchiverReader &reader, bool &arg);
inline ArchiverSizer &operator*(ArchiverSizer &sizer, const bool &arg);

// operator * for array
template <typename T, int N>
ArchiverWriter &operator*(ArchiverWriter &writer, const T (&var)[N]) {
    for (int i = 0; i < N; i++) writer *(var[i]);
    return writer;
}
template <typename T, int N>
ArchiverReader &operator*(ArchiverReader &reader, T (&var)[N]) {
    for (int i = 0; i < N; i++) reader *(var[i]);
    return reader;
}
template <typename T, int N>
ArchiverSizer &operator*(ArchiverSizer &sizer, const T (&var)[N]) {
    for (int i = 0; i < N; i++) sizer *(var[i]);
    return sizer;
}

// operator * for STL array
template <typename T, int N>
ArchiverWriter &operator*(ArchiverWriter &writer, const std::array<T, N> &var) {
    for (int i = 0; i < N; i++) writer *var[i];
    return writer;
}
template <typename T, int N>
ArchiverReader &operator*(ArchiverReader &reader, std::array<T, N> &var) {
    for (int i = 0; i < N; i++) reader *var[i];
    return reader;
}
template <typename T, int N>
ArchiverSizer &operator*(ArchiverSizer &sizer, const std::array<T, N> &var) {
    for (int i = 0; i < N; i++) sizer *var[i];
    return sizer;
}

/**
 * string specialization
 */
inline ArchiverWriter &operator*(ArchiverWriter &writer, const std::string &var) {
    size_t s = var.size();
    if (s > 10000) {
        printf("size (%zu) too big for string when writing archive", s);
    }
    writer *s;
    writer.append_data(var.c_str(), s);
    return writer;
}

inline ArchiverReader &operator*(ArchiverReader &reader, std::string &var) {
    size_t s = 0;
    reader *s;
    if (s > 10000) {
        printf("size (%zu) too big for string when reading archive", s);
    }
    var.resize(s);
    reader.read_data((void *)var.c_str(), s);
    return reader;
}

inline ArchiverSizer &operator*(ArchiverSizer &sizer, const std::string &var) {
    sizer.addSize(sizeof(size_t) + var.size());
    return sizer;
}

// vector
template <typename T>
ArchiverWriter &operator*(ArchiverWriter &writer, const std::vector<T> &var) {
    size_t s = var.size();
    writer *s;
    for (size_t i = 0; i < s; i++) writer *var[i];
    return writer;
}
template <typename T>
ArchiverReader &operator*(ArchiverReader &reader, std::vector<T> &var) {
    size_t s = 0;
    reader *s;
    var.clear();
    if (s > 0) {
        var.resize(s);
        for (size_t i = 0; i < s; i++) reader *var[i];
    }
    return reader;
}
template <typename T>
ArchiverSizer &operator*(ArchiverSizer &sizer, const std::vector<T> &var) {
    size_t s = var.size();
    sizer *s;
    if (s > 0) {
        for (size_t i = 0; i < s; i++) sizer *var[i];
    }
    return sizer;
}

// unordered_set
template <typename T>
inline ArchiverWriter &operator*(ArchiverWriter &writer, const std::unordered_set<T> &var) {
    size_t s = var.size();
    writer *s;
    for (auto it = var.begin(); it != var.end(); it++) writer *(*it);
    return writer;
}
template <typename T>
inline ArchiverReader &operator*(ArchiverReader &reader, std::unordered_set<T> &var) {
    size_t s = 0;
    reader *s;
    var.clear();
    if (s > 0) {
        for (size_t i = 0; i < s; i++) {
            T v;
            reader *v;
            var.insert(v);
        }
    }
    return reader;
}
template <typename T>
inline ArchiverSizer &operator*(ArchiverSizer &sizer, const std::unordered_set<T> &var) {
    size_t s = var.size();
    sizer *s;
    if (s > 0) {
        for (auto it = var.begin(); it != var.end(); it++) sizer *(*it);
    }
    return sizer;
}

// unordered_map
template <typename T1, typename T2>
ArchiverWriter &operator*(ArchiverWriter &writer, const std::unordered_map<T1, T2> &var) {
    size_t s = var.size();
    writer *s;
    for (auto it = var.begin(); it != var.end(); it++) writer *(it->first) * (it->second);
    return writer;
}
template <typename T1, typename T2>
ArchiverReader &operator*(ArchiverReader &reader, std::unordered_map<T1, T2> &var) {
    size_t s = 0;
    reader *s;
    var.clear();
    if (s > 0) {
        for (size_t i = 0; i < s; i++) {
            T1 key;
            T2 value;
            reader *key *value;
            var[key] = value;
        }
    }
    return reader;
}
template <typename T1, typename T2>
ArchiverSizer &operator*(ArchiverSizer &sizer, const std::unordered_map<T1, T2> &var) {
    size_t s = var.size();
    sizer *s;
    if (s > 0) {
        for (auto it = var.begin(); it != var.end(); it++) sizer *(it->first) * (it->second);
    }
    return sizer;
}

// map
template <typename T1, typename T2>
ArchiverWriter &operator*(ArchiverWriter &writer, const std::map<T1, T2> &var) {
    size_t s = var.size();
    writer *s;
    for (auto it = var.begin(); it != var.end(); it++) writer *(it->first) * (it->second);
    return writer;
}
template <typename T1, typename T2>
ArchiverReader &operator*(ArchiverReader &reader, std::map<T1, T2> &var) {
    size_t s = 0;
    reader *s;
    var.clear();
    if (s > 0) {
        for (size_t i = 0; i < s; i++) {
            T1 key;
            T2 value;
            reader *key *value;
            var[key] = value;
        }
    }
    return reader;
}
template <typename T1, typename T2>
ArchiverSizer &operator*(ArchiverSizer &sizer, const std::map<T1, T2> &var) {
    size_t s = var.size();
    sizer *s;
    if (s > 0) {
        for (auto it = var.begin(); it != var.end(); it++) sizer *(it->first) * (it->second);
    }
    return sizer;
}

// stack
template <typename T>
ArchiverWriter &operator*(ArchiverWriter &writer, const std::stack<T> &var) {
    std::stack<T> backup = var;
    size_t s = backup.size();
    std::vector<T> v(s);
    for (size_t i = 0; i < s; i++) {
        v[i] = backup.top();
        backup.pop();
    }
    writer *v;
    return writer;
}
template <typename T>
ArchiverReader &operator*(ArchiverReader &reader, std::stack<T> &var) {
    std::vector<T> v;
    reader *v;
    // while (!var.empty()) var.pop();
    var = std::stack<T>();  // clear stack
    size_t s = v.size();
    if (s > 0) {
        for (size_t i = 0; i < s; i++) {
            var.push(v[s - 1 - i]);
        }
    }
    return reader;
}
template <typename T>
ArchiverSizer &operator*(ArchiverSizer &sizer, const std::stack<T> &var) {
    std::stack<T> backup = var;
    size_t s = backup.size();
    sizer *s;
    for (size_t i = 0; i < s; i++) {
        sizer *backup.top();
        backup.pop();
    }
    return sizer;
}

// deque
template <typename T>
ArchiverWriter &operator*(ArchiverWriter &writer, const std::deque<T> &var) {
    size_t s = var.size();
    writer *s;
    for (size_t i = 0; i < s; i++) writer *var[i];
    return writer;
}
template <typename T>
ArchiverReader &operator*(ArchiverReader &reader, std::deque<T> &var) {
    size_t s = 0;
    reader *s;
    var.clear();
    if (s > 0) {
        var.resize(s);
        for (size_t i = 0; i < s; i++) reader *var[i];
    }
    return reader;
}
template <typename T>
ArchiverSizer &operator*(ArchiverSizer &sizer, const std::deque<T> &var) {
    size_t s = var.size();
    sizer *s;
    if (s > 0) {
        for (size_t i = 0; i < s; i++) sizer *var[i];
    }
    return sizer;
}

// list
template <typename T>
ArchiverWriter &operator*(ArchiverWriter &writer, const std::list<T> &var) {
    size_t s = var.size();
    writer *s;
    for (auto it = var.begin(); it != var.end(); it++) writer *(*it);
    return writer;
}
template <typename T>
ArchiverReader &operator*(ArchiverReader &reader, std::list<T> &var) {
    size_t s = 0;
    reader *s;
    var.clear();
    if (s > 0) {
        for (size_t i = 0; i < s; i++) {
            T v;
            reader *v;
            var.push_back(v);
        }
    }
    return reader;
}
template <typename T>
ArchiverSizer &operator*(ArchiverSizer &sizer, const std::list<T> &var) {
    size_t s = var.size();
    sizer *s;
    for (auto it = var.begin(); it != var.end(); it++) sizer *(*it);
    return sizer;
}

// operator * for arbitrary type
template <typename T>
ArchiverWriter &operator*(ArchiverWriter &writer, const T &var) {
    const T *p = &var;  // use pointer, so that derived function can be called
    p->checkpoint_save(writer);
    return writer;
}
template <typename T>
ArchiverReader &operator*(ArchiverReader &reader, T &var) {
    T *p = &var;  // use pointer, so that derived function can be called
    p->checkpoint_load(reader);
    return reader;
}
template <typename T>
ArchiverSizer &operator*(ArchiverSizer &sizer, const T &var) {
    // use pointer, so that derived function can be called
    const T *p = &var;
    p->checkpoint_bytes(sizer);
    return sizer;
}

// define * for each basic data type
CHECKPOINT_SUPPORT_STRUCT(char)
CHECKPOINT_SUPPORT_STRUCT(unsigned char)
CHECKPOINT_SUPPORT_STRUCT(short)
CHECKPOINT_SUPPORT_STRUCT(unsigned short)
CHECKPOINT_SUPPORT_STRUCT(unsigned int)
CHECKPOINT_SUPPORT_STRUCT(unsigned long long)
CHECKPOINT_SUPPORT_STRUCT(long long)
CHECKPOINT_SUPPORT_STRUCT(float)
CHECKPOINT_SUPPORT_STRUCT(double)
CHECKPOINT_SUPPORT_STRUCT(size_t)
CHECKPOINT_SUPPORT_STRUCT(int64_t)
// CHECKPOINT_SUPPORT_STRUCT(int) // below is expanded example

inline ArchiverWriter &operator*(ArchiverWriter &writer, const int &arg) {
    writer.append_data(&arg, sizeof(int));
    return writer;
}
inline ArchiverReader &operator*(ArchiverReader &reader, int &arg) {
    reader.read_data(&arg, sizeof(int));
    return reader;
}
inline ArchiverSizer &operator*(ArchiverSizer &sizer, const int &arg) {
    sizer.addSize(sizeof(arg));
    return sizer;
}
inline ArchiverWriter &operator*(ArchiverWriter &writer, const std::vector<int> &arg) {
    size_t s = arg.size();
    writer *s;
    writer.append_data(&arg[0], sizeof(int) * s);
    return writer;
}
inline ArchiverReader &operator*(ArchiverReader &reader, std::vector<int> &arg) {
    size_t s;
    reader *s;
    arg.resize(s);
    reader.read_data(&arg[0], sizeof(int) * s);
    return reader;
}
inline ArchiverSizer &operator*(ArchiverSizer &sizer, const std::vector<int> &arg) {
    sizer.addSize(sizeof(size_t) + arg.size() * sizeof(int));
    return sizer;
}

/**
 * bool specialization, as vector<bool> is optimized using bit, not byte
 */
inline ArchiverWriter &operator*(ArchiverWriter &writer, const bool &arg) {
    writer.append_data(&arg, sizeof(bool));
    return writer;
}

inline ArchiverReader &operator*(ArchiverReader &reader, bool &arg) {
    reader.read_data(&arg, sizeof(bool));
    return reader;
}
inline ArchiverSizer &operator*(ArchiverSizer &sizer, const bool &arg) {
    sizer.addSize(sizeof(arg));
    return sizer;
}
}
