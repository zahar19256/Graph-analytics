#pragma once
#include "CsvReader.h"
#include "FileFormat.h"
#include "MmapWriter.hpp"
#include "MyReader.h"
#include "RawReader.hpp"
#include "RawWriter.hpp"

#include <algorithm>
#include <map>
#include <string>
#include <vector>

static const size_t kEdgeBufferSize = (1 << 20);
static const size_t kMaxBucketCount = (1 << 12);

class Convertor {
public:
  Convertor() = default;
  void ToBinaryConvertation(const std::string &input_file,
                            const std::string &output_file);
  void SetupVertexFile(const std::string &output_file);
  void Convertation(const std::string &input_file,
                    const std::string &output_file);

private:
  void BuildMetaFromRaw(const std::string &raw_file);
  void WriteEdges(const std::vector<Edge> &edges, MmapWriter &writer);
  void WriteMeta(MmapWriter &writer, size_t meta_offset) const;
  size_t GetMetaSize() const;
  std::map<int64_t, int64_t> batch_size_;
  MetaData meta_;
};
