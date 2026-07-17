#pragma once
#include <stdexcept>
#include <stdint.h>
#include <filesystem>
#include <fstream>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>
#include "../Base/Graph.h"

static const size_t kThreads = 2;
static const size_t kBatchSize = 128;
static const size_t kEdgeBatchSize = 8192;
static const size_t kPartishionSize = 1e6;

class CsvReader {
public:
    CsvReader() = default;
    CsvReader(std::string path) {
        if (!std::filesystem::exists(path)) {
            throw(std::runtime_error("No input CSV!"));
        }
        input_fd_ = open(path.data() , O_RDONLY);
        if (input_fd_ == -1) {
            throw(std::runtime_error("Cant open input file: " + path + " !"));
        }
        struct stat info {};
        if (fstat(input_fd_, &info) == -1) {
            close(input_fd_);
            throw std::runtime_error("Failed to get input file size!");
        }
        size_ = info.st_size;
        ptr_ = mmap(nullptr , size_ , PROT_READ , MAP_PRIVATE , input_fd_, 0);
        if (ptr_ == MAP_FAILED) {
            throw std::runtime_error("Failed to mmap input file!");
        }
    }
    void ReadNext(std::vector<Edge>& storage);
    bool Empty() const;
private:
    int input_fd_ = -1;
    void* ptr_ = nullptr;
    size_t offset_ = 8;
    size_t size_ = 0;
};
