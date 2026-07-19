#pragma once
#include "../IO/GraphConvertor.h"

#include <string>

static const size_t kMaxRepeats = 100;
static const double kTrashold = 1e-4;

class Executor {
public:
  Executor() = delete;
  Executor(std::string input_file, std::string output_file) {
    Convertor convert;
    convert.Convertation(input_file, output_file);
    edge_file_ = output_file + ".graphZ";
    vertex_file_ = output_file + ".vertex_info";
    result_file_ = output_file + ".leaderrank_reults.csv";
  }
  void Compute();

private:
  void MakeResult(double ground_rank);
  std::string edge_file_;
  std::string vertex_file_;
  std::string result_file_;
};
