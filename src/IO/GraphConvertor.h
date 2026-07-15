#pragma once
#include "MyReader.h"
#include "CsvReader.h"
#include "MyWriter.h"
#include "RawWriter.hpp"
#include "RawReader.h"
#include <string>
#include <vector>
#include <map>

static const size_t kFromBatchSize = (1 << 22);
static const size_t kToBatchSize = (1 << 22);

class Convertor {
public:
    Convertor();
    void ToBinaryConvertation(std::string input_file, std::string output_file);
    void Convertation();
private:
    std::map<std::pair<int32_t , int32_t> , size_t> batch_size_;
};
