#pragma once

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

class RawWriter {
public:
    RawWriter(const std::string& path) {
        std::filesystem::path dest_path(path);
        if (dest_path.has_parent_path()) {
            std::filesystem::create_directories(dest_path.parent_path());
        }
        out_.open(dest_path, std::ios::binary | std::ios::out | std::ios::trunc);
        if (!out_.is_open()) {
            throw std::runtime_error("Cant open output file: " + path + " !");
        }
    }
    void WriteRawBytes(const char* ptr, std::size_t size) {
        if (size == 0) {
            return;
        }
        out_.write(ptr, static_cast<std::streamsize>(size));
        if (!out_) {
            throw std::runtime_error("Failed to write raw bytes!");
        }
    }

    template <class T>
    void Write(const T& value) {
        WriteRawBytes(reinterpret_cast<const char*>(&value), sizeof(T));
    }
    template <class T>
    void Write(const std::vector<T>& values) {
        if (values.empty()) {
            return;
        }
        WriteRawBytes(reinterpret_cast<const char*>(values.data()) , values.size() * sizeof(T));
    }

    void Flush() {
        out_.flush();
        if (!out_) {
            throw std::runtime_error("Failed to flush output file!");
        }
    }

private:
    std::ofstream out_;
};
