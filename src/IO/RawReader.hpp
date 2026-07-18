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
    std::size_t ReadRawBytes(char* ptr, std::size_t size) {
        if (size == 0) {
            return 0;
        }
        in_.read(ptr, static_cast<std::streamsize>(size));
        return static_cast<std::size_t>(in_.gcount());
    }

    void ReadExactRawBytes(char* ptr, std::size_t size) {
        const std::size_t read_bytes = ReadRawBytes(ptr, size);
        if (read_bytes != size) {
            throw std::runtime_error("Failed to read raw bytes!");
        }
    }

    template <class T>
    bool Read(T& value) {
        return ReadRawBytes(reinterpret_cast<char*>(&value) , sizeof(T)) == sizeof(T);
    }

    template <class T>
    std::size_t ReadVector(std::vector<T>& values, std::size_t count) {
        values.resize(count);
        if (!values.empty()) {
            const std::size_t read_bytes = ReadRawBytes(
                reinterpret_cast<char*>(values.data()),
                values.size() * sizeof(T)
            );
            values.resize(read_bytes / sizeof(T));
        }
        return values.size();
    }

    template <class T>
    std::size_t ReadVector(std::vector<T>& values) {
        return ReadVector(values, values.size());
    }

    template <class T>
    std::vector<T> ReadVector(std::size_t count) {
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
