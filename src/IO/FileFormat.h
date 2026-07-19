#pragma once

#include <stdint.h>
#include <cstddef>

static const size_t kFromBatchSize = (1 << 20);
static const size_t kToBatchSize = (1 << 20);
static const size_t kEdgeReadBatchSize = (1 << 20);

struct GraphMetaHeader {
    uint64_t edge_count;
    uint64_t batch_count;
    uint64_t vertex_count;
    uint64_t from_batch_size;
    uint64_t to_batch_size;
};

struct GraphMetaBatchRecord {
    int64_t index;
    uint64_t offset;
    uint64_t size;
};

struct VertexInfo {
    int32_t old_id;
    int32_t out_degree;
    float rank;
    float next_rank;
};
