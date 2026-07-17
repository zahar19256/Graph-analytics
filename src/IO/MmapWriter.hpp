#pragma once
#include <cstddef>
#include <cstring>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

class MmapWriter {
public:
    MmapWriter(const std::string& path, size_t size) : size_(size) {
        std::filesystem::path dest_path(path);
        if (dest_path.has_parent_path()) {
            std::filesystem::create_directories(dest_path.parent_path());
        }
        fd_ = open(path.c_str(), O_RDWR | O_CREAT, 0644);
        if (fd_ == -1) {
            throw std::runtime_error("Fail to open output file: " + path);
        }
        if (ftruncate(fd_, size_) == -1) {
            close(fd_);
            throw std::runtime_error("Fail to set file size!");
        }
        data_ = static_cast<char*>(mmap(nullptr, size_, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0));
        if (data_ == MAP_FAILED) {
            close(fd_);
            throw std::runtime_error("Failed to mmap file: " + path);
        }
    }
    ~MmapWriter() {
        if (data_ != MAP_FAILED) {
            munmap(data_, size_);
        }
        if (fd_ != -1) {
            close(fd_);
        }
    }
    MmapWriter(const MmapWriter&) = delete;
    MmapWriter& operator=(const MmapWriter&) = delete;

    void WriteRawBytes(size_t offset , const char* ptr , size_t len) {
        if (len == 0) {
            return;
        }
        if (offset + len > size_) {
            throw std::runtime_error("Tru to write out of file bounds!");
        }
        memcpy(data_ + offset , ptr , len);
    }

    template <class T>
    void Write(size_t offset , const T& value) {
        WriteRawBytes(offset , reinterpret_cast<const char*>(&value) , sizeof(T));
    }

    template <class T>
    void Write(size_t offset , const std::vector<T>& values) {
        if (values.empty()) {
            return;
        }
        WriteRawBytes(offset , reinterpret_cast<const char*>(values.data()) , values.size() * sizeof(T));
    }

    char* Data() {
        return data_;
    }

    void Flush() {
        if (msync(data_ , size_ , MS_SYNC) == -1) {
            throw std::runtime_error("Failed to flush output file!");
        }
    }

private:
    size_t size_ = 0;
    int fd_ = -1;
    char* data_;
};