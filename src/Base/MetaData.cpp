#include "MetaData.h"
#include <cstdint>
#include <stdexcept>

void MetaData::Clear() {
    batch_offset_.clear();
    total_edge_count_ = 0;
    vertex_count_ = 0;
}

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

bool MetaData::Contains(int64_t index) const {
    return batch_offset_.count(index);
}

size_t MetaData::GetOffset(int64_t index) const {
    std::unordered_map<int64_t, batch_info>::const_iterator it = batch_offset_.find(index);
    if (it == batch_offset_.end()) {
        throw std::runtime_error("No such index in meta data batch_info container: " + std::to_string(index));
    }
    return it->second.offset;
}

size_t MetaData::GetUsedSize(int64_t index) const {
    std::unordered_map<int64_t, batch_info>::const_iterator it = batch_offset_.find(index);
    if (it == batch_offset_.end()) {
        throw std::runtime_error("No such index in meta data batch_info container: " + std::to_string(index));
    }
    return it->second.used_size;
}

size_t MetaData::GetSize(int64_t index) const {
    std::unordered_map<int64_t, batch_info>::const_iterator it = batch_offset_.find(index);
    if (it == batch_offset_.end()) {
        throw std::runtime_error("No such index in meta data batch_info container: " + std::to_string(index));
    }
    return it->second.size;
}

size_t MetaData::GetEdgeCount() const {
    return total_edge_count_;
}

size_t MetaData::GetVertexCount() const {
    return vertex_count_;
}

void MetaData::SetVertexCount(size_t vertex_count) {
    vertex_count_ = vertex_count;
}

void MetaData::TouchVertex(size_t vertex) {
    if (vertex + 1 > vertex_count_) {
        vertex_count_ = vertex + 1;
    }
}

const std::unordered_map<int64_t, batch_info>& MetaData::GetBatches() const {
    return batch_offset_;
}
