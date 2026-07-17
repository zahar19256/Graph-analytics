#pragma once
#include <vector>
#include <unordered_map>
#include <stdint.h>

struct batch_info {
    size_t offset;
    size_t size;
};

class MetaData {
public:
    void InsertInfo(int64_t index , batch_info info);
    size_t GetSize(int64_t index);
    size_t GetOffset(int64_t index);
    size_t GetEdgeCount();
private:
    std::unordered_map<int64_t, batch_info> batch_offset_;
    size_t total_edge_count_;
};
