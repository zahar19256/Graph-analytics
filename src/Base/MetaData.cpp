#include "MetaData.h"
#include <cstdint>
#include <stdexcept>

void MetaData::InsertInfo(int64_t index , batch_info info) {
    batch_offset_[index] = info;
    total_edge_count_ += info.size;
}

void MetaData::FillBatch(int64_t index , size_t size) {
    if (!batch_offset_.count(index)) {
        throw std::runtime_error("No such index in meta data batch_info container: " + std::to_string(index));
    }
    if (batch_offset_[index].used_size + size > batch_offset_[index].size) {
        throw std::runtime_error("No space left in batch: " + std::to_string(index));
    }
    batch_offset_[index].used_size += size;
}

size_t MetaData::GetOffset(int64_t index) {
    if (!batch_offset_.count(index)) {
        throw std::runtime_error("No such index in meta data batch_info container: " + std::to_string(index));
    }
    return batch_offset_[index].offset;
}

size_t MetaData::GetSize(int64_t index) {
    if (!batch_offset_.count(index)) {
        throw std::runtime_error("No such index in meta data batch_info container: " + std::to_string(index));
    }
    return batch_offset_[index].size;
}

size_t MetaData::GetEdgeCount() {
    return total_edge_count_;
}
