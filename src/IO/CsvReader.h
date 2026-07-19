#pragma once
#include "../Base/Graph.h"
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <stdint.h>
#include <string>
#include <vector>

static const size_t kThreads = 2;
static const size_t kBatchSize = 128;
static const size_t kEdgeBatchSize = 8192;
static const size_t kPartishionSize = 1e6;

class CsvReader {
public:
  CsvReader() = default;
  CsvReader(std::string path);
  void ReadNext(std::vector<Edge> &storage);
  bool Empty() const;

private:
  std::ifstream input_;
  Edge pending_edge_{};
  bool on_edge_ = false;
  bool empty_ = true;
};
