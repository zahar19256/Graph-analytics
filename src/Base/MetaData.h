#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>
#include <unordered_map>

struct batch_info {
    size_t offset;
    size_t size;
    size_t used_size;
};

class MetaData {
public:
    void InsertInfo(int64_t index , batch_info info);
    void FillBatch(int64_t index, size_t size);
    size_t GetSize(int64_t index);
    size_t GetOffset(int64_t index);
    size_t GetEdgeCount();
private:
    std::unordered_map<int64_t, batch_info> batch_offset_;
    size_t total_edge_count_ = 0;
};
