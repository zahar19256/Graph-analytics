#pragma once
#include <stdint.h>
#include <vector>
#include <filesystem>
#include <fstream>
#include <sys/mman.h>
#include "../Base/Graph.h"

static const size_t kThreads = 2;
static const size_t kBatchSize = 128;
static const size_t kPartishionSize = 1e6;

namespace fs = std::filesystem;

class Reader {
public:
    Reader() = default;
    Reader(const fs::path& path) {
        
    }
private:
    std::vector<Edge> buffer_;
    
};