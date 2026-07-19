#pragma once
#include <cstddef>
#include <cstdint>
#include <unordered_map>

struct batch_info {
  size_t offset;
  size_t size;
  size_t used_size;
};

class MetaData {
public:
  void Clear();
  void InsertInfo(int64_t index, batch_info info);
  void FillBatch(int64_t index, size_t size);
  bool Contains(int64_t index) const;
  size_t GetSize(int64_t index) const;
  size_t GetOffset(int64_t index) const;
  size_t GetUsedSize(int64_t index) const;
  size_t GetEdgeCount() const;
  size_t GetVertexCount() const;
  void SetVertexCount(size_t vertex_count);
  void TouchVertex(size_t vertex);
  const std::unordered_map<int64_t, batch_info> &GetBatches() const;

private:
  std::unordered_map<int64_t, batch_info> batch_offset_;
  size_t total_edge_count_ = 0;
  size_t vertex_count_ = 0;
};
