#include "CsvReader.h"

bool ReadInt32(const std::string &line, size_t &position, int32_t &value) {
  while (position < line.size() && line[position] != '-' &&
         (line[position] < '0' || line[position] > '9')) {
    ++position;
  }
  if (position == line.size()) {
    return false;
  }

  int32_t sign = 1;
  if (line[position] == '-') {
    sign = -1;
    ++position;
  }

  int32_t result = 0;
  while (position < line.size() && line[position] >= '0' &&
         line[position] <= '9') {
    result = result * 10 + (line[position] - '0');
    ++position;
  }

  value = result * sign;
  return true;
}

bool ReadEdge(const std::string &line, Edge &edge) {
  size_t position = 0;
  int32_t from = 0;
  int32_t to = 0;
  if (!ReadInt32(line, position, from)) {
    return false;
  }
  if (!ReadInt32(line, position, to)) {
    return false;
  }
  edge = {from, to};
  return true;
}

CsvReader::CsvReader(std::string path) {
  if (!std::filesystem::exists(path)) {
    throw(std::runtime_error("No input CSV!"));
  }
  input_.open(path, std::ios::in);
  if (!input_.is_open()) {
    throw(std::runtime_error("Cant open input file: " + path + " !"));
  }
  std::string line;
  if (!std::getline(input_, line)) {
    empty_ = true;
    return;
  }
  empty_ = false;
  if (ReadEdge(line, pending_edge_)) {
    on_edge_ = true;
  }
}

void CsvReader::ReadNext(std::vector<Edge> &storage) {
  storage.clear();
  size_t count = 0;
  if (on_edge_) {
    storage.push_back(pending_edge_);
    on_edge_ = false;
    ++count;
  }
  std::string line;
  while (count < kEdgeBatchSize && std::getline(input_, line)) {
    Edge edge{};
    if (ReadEdge(line, edge)) {
      storage.push_back(edge);
      ++count;
    }
  }
  if (storage.empty() && input_.eof()) {
    empty_ = true;
  }
  if (!on_edge_ && input_.eof()) {
    empty_ = true;
  }
}

bool CsvReader::Empty() const { return empty_; }
