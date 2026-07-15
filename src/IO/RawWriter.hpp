#pragma once
#include <fstream>
#include <filesystem>

class RawWriter {
public:
    RawWriter(std::string path) {
        std::filesystem::path dest_path(path);
        if (dest_path.has_parent_path()) {
            std::filesystem::create_directories(dest_path.parent_path());
        }
        fout_.open(dest_path , std::ios::binary | std::ios::out);
    }
    void WriteRawBytes(const char* ptr , size_t size) {
        out_.write(ptr , size);
    }
private:
    std::ofstream out_;
};
