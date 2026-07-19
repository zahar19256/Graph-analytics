#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

class RawReader {
public:
    RawReader(const std::string& path) {
        if (!std::filesystem::exists(path)) {
            throw std::runtime_error("No input raw file: " + path + " !");
        }
        in_.open(path, std::ios::binary | std::ios::in);
        if (!in_.is_open()) {
            throw std::runtime_error("Cant open input file: " + path + " !");
        }
    }
    size_t ReadRawBytes(char* ptr, size_t size) {
        if (size == 0) {
            return 0;
        }
        in_.read(ptr, size);
        return static_cast<size_t>(in_.gcount());
    }

    void Seek(size_t offset) {
        in_.clear();
        in_.seekg(offset, std::ios::beg);
    }

    void ReadExactRawBytes(char* ptr, size_t size) {
        size_t read_bytes = ReadRawBytes(ptr, size);
        if (read_bytes != size) {
            throw std::runtime_error("Failed to read raw bytes!");
        }
    }

    template <typename T>
    bool Read(T& value) {
        return ReadRawBytes(reinterpret_cast<char*>(&value) , sizeof(T)) == sizeof(T);
    }

    template <typename T>
    size_t ReadVector(std::vector<T>& values, size_t count) {
        values.resize(count);
        if (!values.empty()) {
            size_t read_bytes = ReadRawBytes(
                reinterpret_cast<char*>(values.data()),
                values.size() * sizeof(T)
            );
            values.resize(read_bytes / sizeof(T));
        }
        return values.size();
    }

    template <typename T>
    size_t ReadVector(std::vector<T>& values) {
        return ReadVector(values, values.size());
    }

    template <typename T>
    std::vector<T> ReadVector(size_t count) {
        std::vector<T> values;
        ReadVector(values, count);
        return values;
    }

    bool Empty() {
        return in_.peek() == std::ifstream::traits_type::eof();
    }

private:
    std::ifstream in_;
};
