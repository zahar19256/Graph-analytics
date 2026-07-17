#pragma once
#include "MyReader.h"
#include "CsvReader.h"
#include "MmapWriter.hpp"
#include "RawWriter.hpp"
#include "RawReader.h"
#include <string>
#include <vector>
#include <map>

static const size_t kFromBatchSize = (1 << 22);
static const size_t kToBatchSize = (1 << 22);
static const size_t kEdgeBufferSize = (1 << 22);

class Convertor {
public:
    Convertor();
    void ToBinaryConvertation(std::string input_file, std::string output_file);
    void Convertation(std::string input_file, std::string output_file);
private:
    std::map<int64_t , int64_t> batch_size_;
    MetaData meta_;
};
