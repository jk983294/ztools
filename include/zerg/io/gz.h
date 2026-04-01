#pragma once

#include <zlib.h>
#include <string>
#include <vector>
#include <zerg/log.h>

/**
 * typical usage:
 *
    GzBufferWriter<FuturesMarketDataFieldL1> writer;
    GzBufferReader<FuturesMarketDataFieldL1> reader;
    if (!reader.open(p_)) {
        cerr << "cannot open " << p_ << endl;
        return;
    }

    if (!writer.open(fpath_to)) {
        cerr << "cannot open " << fpath_to << endl;
        return;
    }

    while (!reader.iseof()) {
        int num = reader.read();
        if (num < 0) {
            cerr << "error when reading " << p_ << endl;
            abort();
        }
        for (int i = 0; i < num; ++i) {
            auto &data = reader.buffer[i];

            // work on data

            writer.append(data);
        }

        int ret_val = writer.flush();
        if(ret_val < 0) {
            LOG_AND_THROW("write md failed, ret=%d", ret_val);
        }
    }
 */
namespace zerg {
template <typename T>
struct GzBufferReader {
    T* buffer{nullptr};
    int len = 1000;
    int current_total{0};
    int next_idx{0};
    gzFile zp{nullptr};
    std::string path;

    GzBufferReader() { init(); }
    GzBufferReader(int len_) : len{len_} { init(); }
    ~GzBufferReader() {
        delete[] buffer;
        if (zp) gzclose(zp);
    }

    bool open() { return open(path); }

    bool open(const std::string& path_) {
        if (&path != &path_) path = path_;
        if (zp) {
            gzclose(zp);
        }
        zp = gzopen(path_.c_str(), "rb");
        return zp != nullptr;
    }

    int read() {
        if (gzeof(zp)) return 0;
        int ret_val = gzread(zp, buffer, sizeof(T) * len);
        if (ret_val < 0) {
            return -1;
        }
        current_total = ret_val / sizeof(T);
        next_idx = 0;
        return current_total;
    }

    bool iseof() { return gzeof(zp); }

    T* get_one() {
        if(next_idx < current_total) return buffer + next_idx;
        else return nullptr;
    }

    void advance_one() {
        ++next_idx;
    }

private:
    void init() { buffer = new T[len]; }
};

template <typename T>
struct GzBufferWriter {
    T* buffer{nullptr};
    int len = 100, used_count = 0;
    gzFile zp{nullptr};
    std::string path;

    GzBufferWriter() { init(); }
    GzBufferWriter(int len_) : len{len_} { init(); }
    ~GzBufferWriter() {
        delete[] buffer;
        if (zp) gzclose(zp);
    }

    bool open() { return open(path); }

    bool open(const std::string& path_) {
        if (&path != &path_) path = path_;
        if (zp) {
            gzclose(zp);
        }
        zp = gzopen(path_.c_str(), "wb");
        return zp != nullptr;
    }

    int write(const T& d) {
        buffer[used_count++] = d;
        return flush();
    }

    /**
     * this function can handle outside pData which is zero memory copy
     */
    int flush(const T* pData, int count) {
        int ret_val = gzwrite(zp, pData, sizeof(T) * count);
        if (ret_val < 0) {
            return -1;
        }
        int write_count = ret_val / sizeof(T);
        if (write_count < count) {
            return -write_count;
        }
        return write_count;
    }

    int flush() {
        if (used_count == 0) return 0;
        int ret_val = flush(buffer, used_count);
        if (ret_val == used_count) {
            used_count = 0;
            return ret_val;
        } else {
            return ret_val;
        }
    }

    /**
     * 和 flush() 配合使用，先写buffer再一把存磁盘
     */
    int append(const T& d) {
        buffer[used_count++] = d;
        if (used_count == len) {
            return flush();
        }
        return 1;
    }

private:
    void init() { buffer = new T[len]; }
};

inline std::vector<std::string> read_gz_file(const std::string& file_name) {
  std::vector<std::string> lines;
  gzFile gz_file = gzopen(file_name.c_str(), "rb");
  if (!gz_file) {
    ZLOG_THROW("Failed to open gz file: %s", file_name.c_str());
  }

  char buffer[8192];
  std::string current_line;
  int bytes_read;
  while ((bytes_read = gzread(gz_file, buffer, sizeof(buffer))) > 0) {
    for (int i = 0; i < bytes_read; ++i) {
      if (buffer[i] == '\n') {
        // Trim trailing \r if present (Windows line endings)
        if (!current_line.empty() && current_line.back() == '\r') {
          current_line.pop_back();
        }
        lines.push_back(current_line);
        current_line.clear();
      } else if (buffer[i] != '\r') {
        current_line += buffer[i];
      }
    }
  }

  // Handle last line if file doesn't end with newline
  if (!current_line.empty()) {
    if (current_line.back() == '\r') {
      current_line.pop_back();
    }
    lines.push_back(current_line);
  }

  gzclose(gz_file);
  return lines;
}
}