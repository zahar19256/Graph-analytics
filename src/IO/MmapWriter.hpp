#pragma once
#include <cstddef>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <limits>
#include <stdexcept>
#include <string>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

class MmapWriter {
public:
  MmapWriter(const std::string &path, size_t size) : size_(size) {
    std::filesystem::path dest_path(path);
    if (dest_path.has_parent_path()) {
      std::filesystem::create_directories(dest_path.parent_path());
    }
    fd_ = open(path.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd_ == -1) {
      throw std::runtime_error("Fail to open output file: " + path);
    }
    if (ftruncate(fd_, size_) == -1) {
      close(fd_);
      throw std::runtime_error("Fail to set file size!");
    }
    if (size_ == 0) {
      return;
    }
    void* ptr =
        mmap(nullptr, size_, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0);
    if (ptr == MAP_FAILED) {
      close(fd_);
      throw std::runtime_error("Fail to mmap output file: " + path);
    }
    data_ = static_cast<char*>(ptr);
  }
  ~MmapWriter() {
    if (data_ != nullptr) {
      msync(data_, size_, MS_SYNC);
      munmap(data_, size_);
    }
    if (fd_ != -1) {
      close(fd_);
    }
  }
  MmapWriter(const MmapWriter &) = delete;
  MmapWriter &operator=(const MmapWriter &) = delete;

  void WriteRawBytes(size_t offset, const char *ptr, size_t len) {
    if (len == 0) {
      return;
    }
    if (offset > size_ || len > size_ - offset) {
      throw std::runtime_error("Try to write out of file bounds!");
    }
    std::memcpy(data_ + offset, ptr, len);
  }

  template <typename T> void Write(size_t offset, const T &value) {
    WriteRawBytes(offset, reinterpret_cast<const char *>(&value), sizeof(T));
  }

  template <typename T> void Write(size_t offset, const T *data, size_t count) {
    if (count == 0) {
      return;
    }
    if (count > std::numeric_limits<size_t>::max() / sizeof(T)) {
      throw std::runtime_error("Write size overflow!");
    }
    WriteRawBytes(offset, reinterpret_cast<const char *>(data),
                  count * sizeof(T));
  }

  template <typename T>
  void Write(size_t offset, const std::vector<T> &values) {
    Write(offset, values.data(), values.size());
  }

  void Flush() {
    if (data_ != nullptr && msync(data_, size_, MS_SYNC) == -1) {
      throw std::runtime_error("Failed to sync output file mapping!");
    }
    if (fsync(fd_) == -1) {
      throw std::runtime_error("Failed to flush output file!");
    }
  }

private:
  size_t size_ = 0;
  int fd_ = -1;
  char *data_ = nullptr;
};
