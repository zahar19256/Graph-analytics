#include "MyReader.h"
#include "FileFormat.h"

#include <fcntl.h>
#include <filesystem>
#include <limits>
#include <stdexcept>
#include <sys/stat.h>
#include <unistd.h>

namespace {

std::string ResolveGraphPath(const std::string &path) {
  if (std::filesystem::exists(path)) {
    return path;
  }
  if (std::filesystem::exists(path + ".graphZ")) {
    return path + ".graphZ";
  }
  throw std::runtime_error("No input graph file: " + path + " !");
}

} // namespace

MyReader::MyReader(std::string path) {
  graph_path_ = ResolveGraphPath(path);

  input_fd_ = open(graph_path_.data(), O_RDONLY);
  if (input_fd_ == -1) {
    throw std::runtime_error("Cant open input file: " + graph_path_ + " !");
  }
  struct stat info{};
  if (fstat(input_fd_, &info) == -1) {
    close(input_fd_);
    input_fd_ = -1;
    throw std::runtime_error("Failed to get input file size!");
  }
  size_ = info.st_size;
  ReadMeta();
}

MyReader::~MyReader() {
  if (input_fd_ != -1) {
    close(input_fd_);
  }
}

void MyReader::ReadRawBytes(size_t offset, char *data, size_t size) {
  size_t read_bytes = 0;
  while (read_bytes < size) {
    ssize_t current = pread(input_fd_, data + read_bytes, size - read_bytes,
                            offset + read_bytes);
    if (current <= 0) {
      throw std::runtime_error("Failed to read graph file!");
    }
    read_bytes += static_cast<size_t>(current);
  }
}

void MyReader::ReadBatch(size_t index, std::vector<Edge> &storage) {
  size_t count = meta_.GetSize(static_cast<int64_t>(index));
  ReadBatchPart(index, 0, count, storage);
}

void MyReader::ReadBatchPart(size_t index, size_t edge_offset, size_t count,
                             std::vector<Edge> &storage) {
  size_t batch_size = meta_.GetSize(static_cast<int64_t>(index));
  if (edge_offset >= batch_size) {
    storage.clear();
    return;
  }
  if (edge_offset + count > batch_size) {
    count = batch_size - edge_offset;
  }
  size_t offset = meta_.GetOffset(static_cast<int64_t>(index)) + edge_offset;
  size_t byte_offset = offset * sizeof(Edge);
  size_t byte_size = count * sizeof(Edge);
  size_t edge_bytes = meta_.GetEdgeCount() * sizeof(Edge);
  if (byte_offset > edge_bytes || byte_size > edge_bytes - byte_offset) {
    throw std::runtime_error("Batch is out of graph file bounds!");
  }

  storage.resize(count);
  if (count == 0) {
    return;
  }
  ReadRawBytes(byte_offset, reinterpret_cast<char *>(storage.data()),
               byte_size);
}

void MyReader::ReadEdges(std::vector<Edge> &storage) {
  size_t count = meta_.GetEdgeCount();
  storage.resize(count);
  if (count == 0) {
    return;
  }
  ReadRawBytes(0, reinterpret_cast<char *>(storage.data()),
               count * sizeof(Edge));
}

void MyReader::ReadMeta() {
  if (size_ < sizeof(GraphMetaHeader)) {
    throw std::runtime_error("Graph file is too small to contain metadata!");
  }

  size_t header_offset = size_ - sizeof(GraphMetaHeader);
  GraphMetaHeader header{};
  ReadRawBytes(header_offset, reinterpret_cast<char *>(&header),
               sizeof(header));
  if (header.from_batch_size != kFromBatchSize ||
      header.to_batch_size != kToBatchSize) {
    throw std::runtime_error(
        "Graph meta batch size does not match current binary!");
  }
  if (header.edge_count > std::numeric_limits<size_t>::max() / sizeof(Edge)) {
    throw std::runtime_error("Graph edge section size overflow!");
  }
  if (header.batch_count >
      std::numeric_limits<size_t>::max() / sizeof(GraphMetaBatchRecord)) {
    throw std::runtime_error("Graph meta batch table size overflow!");
  }
  if (header.vertex_count > std::numeric_limits<size_t>::max()) {
    throw std::runtime_error("Graph vertex count overflow!");
  }

  size_t edge_bytes = static_cast<size_t>(header.edge_count) * sizeof(Edge);
  size_t batch_bytes =
      static_cast<size_t>(header.batch_count) * sizeof(GraphMetaBatchRecord);
  if (edge_bytes > size_ || batch_bytes > size_ - edge_bytes ||
      sizeof(GraphMetaHeader) != size_ - edge_bytes - batch_bytes) {
    throw std::runtime_error("Graph metadata layout is invalid!");
  }

  meta_.Clear();
  for (uint64_t i = 0; i < header.batch_count; ++i) {
    GraphMetaBatchRecord record{};
    size_t record_offset = edge_bytes + static_cast<size_t>(i) * sizeof(record);
    ReadRawBytes(record_offset, reinterpret_cast<char *>(&record),
                 sizeof(record));
    meta_.InsertInfo(record.index, {static_cast<size_t>(record.offset),
                                    static_cast<size_t>(record.size), 0});
  }

  meta_.SetVertexCount(static_cast<size_t>(header.vertex_count));

  if (meta_.GetEdgeCount() != static_cast<size_t>(header.edge_count)) {
    throw std::runtime_error("Graph meta edge count mismatch!");
  }
}

size_t MyReader::GetEdgeCount() const { return meta_.GetEdgeCount(); }

size_t MyReader::GetVertexCount() const { return meta_.GetVertexCount(); }

const MetaData &MyReader::GetMetaData() const { return meta_; }
