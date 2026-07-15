#pragma once
#include <vector>
#include <unordered_map>

class MetaData {
public:
    void Append(size_t offset);
    size_t GetSize(size_t index);
    size_t GetOffset(size_t index);
private:
    std::unordered_map<size_t, size_t> batch_offset_;

};
