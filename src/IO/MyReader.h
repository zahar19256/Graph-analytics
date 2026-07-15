#pragma once
#include "../Base/Graph.h"
#include "../Base/MetaData.h"

#include <string>
#include <paths.h>
#include <stdexcept>
#include <stdint.h>
#include <vector>
#include <filesystem>
#include <fstream>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

class MyReader {
public:
    MyReader(std::string path) {
        if (!std::filesystem::exists(path)) {
            throw(std::runtime_error("No input Raw!"));
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
    void ReadBatch(size_t index, std::vector<Edge>& storage);
    void ReadEdges(std::vector<Edge>& storage);
    void ReadMeta();
private:
    void* ptr_;
    int input_fd_;
    size_t size_;
    MetaData meta_;
};
