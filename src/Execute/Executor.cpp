#include "Executor.h"

#include "../IO/FileFormat.h"
#include "../IO/MyReader.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <iostream>
#include <stdexcept>
#include <unordered_map>
#include <vector>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>

namespace {

static const int kOpenMpThreadCount = 2;

struct VertexFile {
    explicit VertexFile(const std::string& path) {
        fd = open(path.data(), O_RDWR);
        if (fd == -1) {
            throw std::runtime_error("Cant open vertex file: " + path);
        }
    }

    ~VertexFile() {
        if (fd != -1) {
            close(fd);
        }
    }

    void ReadChunk(size_t first_vertex, size_t count, std::vector<VertexInfo>& storage) {
        storage.resize(count);
        if (count == 0) {
            return;
        }
        size_t bytes = count * sizeof(VertexInfo);
        ssize_t read_bytes = pread(fd, storage.data(), bytes, first_vertex * sizeof(VertexInfo));
        if (read_bytes < 0 || static_cast<size_t>(read_bytes) != bytes) {
            throw std::runtime_error("Failed to read vertex chunk!");
        }
    }

    void WriteChunk(size_t first_vertex, const std::vector<VertexInfo>& storage) {
        if (storage.empty()) {
            return;
        }
        size_t bytes = storage.size() * sizeof(VertexInfo);
        ssize_t written_bytes = pwrite(fd, storage.data(), bytes, first_vertex * sizeof(VertexInfo));
        if (written_bytes < 0 || static_cast<size_t>(written_bytes) != bytes) {
            throw std::runtime_error("Failed to write vertex chunk!");
        }
    }

    int fd = -1;
};

size_t GetChunkSize(size_t chunk, size_t chunk_size, size_t vertex_count) {
    size_t first_vertex = chunk * chunk_size;
    if (first_vertex + chunk_size <= vertex_count) {
        return chunk_size;
    }
    return vertex_count - first_vertex;
}

} // namespace

void Executor::Compute() {
    MyReader graph_reader(edge_file_);
    VertexFile vertex_file(vertex_file_);

    size_t vertex_count = graph_reader.GetVertexCount();
    if (vertex_count == 0) {
        MakeResult(0.0);
        return;
    }

    size_t to_batch_count = (vertex_count + kToBatchSize - 1) / kToBatchSize;
    std::vector<std::vector<int64_t>> batches_by_to(to_batch_count);
    const std::unordered_map<int64_t, batch_info>& meta_batches = graph_reader.GetMetaData().GetBatches();
    for (std::unordered_map<int64_t, batch_info>::const_iterator it = meta_batches.begin(); it != meta_batches.end(); ++it) {
        size_t to_batch = static_cast<size_t>(it->first / kMaxBucketCount);
        if (to_batch < to_batch_count) {
            batches_by_to[to_batch].push_back(it->first);
        }
    }

    double ground_rank = 0.0;

    for (size_t gen = 0; gen < kMaxRepeats; ++gen) {
        std::chrono::steady_clock::time_point iteration_start = std::chrono::steady_clock::now();
        double ground_next_rank = 0.0;
        double ground_add = ground_rank / static_cast<double>(vertex_count);
        double max_rank_diff = 0.0;

#pragma omp parallel for num_threads(kOpenMpThreadCount) schedule(dynamic) reduction(+:ground_next_rank) reduction(max:max_rank_diff)
        for (long long to_batch_id = 0; to_batch_id < static_cast<long long>(to_batch_count); ++to_batch_id) {
            size_t to_batch = static_cast<size_t>(to_batch_id);
            size_t to_start = to_batch * kToBatchSize;
            size_t to_count = GetChunkSize(to_batch, kToBatchSize, vertex_count);
            std::vector<Edge> edges;
            std::vector<VertexInfo> to_vertices;
            std::vector<VertexInfo> from_vertices;

            vertex_file.ReadChunk(to_start, to_count, to_vertices);

            for (size_t i = 0; i < to_count; ++i) {
                to_vertices[i].next_rank = static_cast<float>(ground_add);
                ground_next_rank += static_cast<double>(to_vertices[i].rank) /
                    static_cast<double>(to_vertices[i].out_degree + 1);
            }

            for (int64_t index : batches_by_to[to_batch]) {
                size_t from_batch = static_cast<size_t>(index % kMaxBucketCount);
                size_t from_start = from_batch * kFromBatchSize;
                size_t from_count = GetChunkSize(from_batch, kFromBatchSize, vertex_count);
                size_t batch_size = graph_reader.GetMetaData().GetSize(index);

                vertex_file.ReadChunk(from_start, from_count, from_vertices);
                for (size_t edge_offset = 0; edge_offset < batch_size; edge_offset += kEdgeReadBatchSize) {
                    size_t edge_count = std::min(kEdgeReadBatchSize, batch_size - edge_offset);
                    graph_reader.ReadBatchPart(static_cast<size_t>(index), edge_offset, edge_count, edges);

                    for (Edge& edge : edges) {
                        size_t from = static_cast<size_t>(edge.from) - from_start;
                        size_t to = static_cast<size_t>(edge.to) - to_start;
                        to_vertices[to].next_rank += static_cast<float>(
                            static_cast<double>(from_vertices[from].rank) /
                            static_cast<double>(from_vertices[from].out_degree + 1)
                        );
                    }
                }
            }

            for (size_t i = 0; i < to_count; ++i) {
                double diff = std::abs(static_cast<double>(to_vertices[i].rank) - static_cast<double>(to_vertices[i].next_rank));
                max_rank_diff = std::max(max_rank_diff, diff);
            }

            vertex_file.WriteChunk(to_start, to_vertices);
        }

#pragma omp parallel for num_threads(kOpenMpThreadCount) schedule(dynamic)
        for (long long to_batch_id = 0; to_batch_id < static_cast<long long>(to_batch_count); ++to_batch_id) {
            size_t to_batch = static_cast<size_t>(to_batch_id);
            size_t to_start = to_batch * kToBatchSize;
            size_t to_count = GetChunkSize(to_batch, kToBatchSize, vertex_count);
            std::vector<VertexInfo> to_vertices;

            vertex_file.ReadChunk(to_start, to_count, to_vertices);
            for (size_t i = 0; i < to_count; ++i) {
                to_vertices[i].rank = to_vertices[i].next_rank;
                to_vertices[i].next_rank = 0;
            }
            vertex_file.WriteChunk(to_start, to_vertices);
        }

        double ground_diff = std::abs(ground_rank - ground_next_rank);
        ground_rank = ground_next_rank;
        std::chrono::steady_clock::time_point iteration_finish = std::chrono::steady_clock::now();
        double seconds = std::chrono::duration<double>(iteration_finish - iteration_start).count();
        std::cerr << "LeaderRank iteration " << gen << " time: " << seconds << " sec" << std::endl;
        if (max_rank_diff <= kTrashold && ground_diff <= kTrashold) {
            break;
        }
    }

    MakeResult(ground_rank);
}

void Executor::MakeResult(double ground_rank) {
    MyReader graph_reader(edge_file_);
    VertexFile vertex_file(vertex_file_);

    size_t vertex_count = graph_reader.GetVertexCount();
    FILE* out = fopen(result_file_.c_str(), "w");
    if (!out) {
        throw std::runtime_error("Failed to open result file: " + result_file_);
    }

    fprintf(out, "vertex,rank\n");
    if (vertex_count == 0) {
        fclose(out);
        return;
    }

    double ground_add = ground_rank / static_cast<double>(vertex_count);
    size_t to_batch_count = (vertex_count + kToBatchSize - 1) / kToBatchSize;
    std::vector<VertexInfo> vertices;
    for (size_t to_batch = 0; to_batch < to_batch_count; ++to_batch) {
        size_t to_start = to_batch * kToBatchSize;
        size_t to_count = GetChunkSize(to_batch, kToBatchSize, vertex_count);
        vertex_file.ReadChunk(to_start, to_count, vertices);
        for (size_t i = 0; i < to_count; ++i) {
            fprintf(out, "%d,%.17g\n", vertices[i].old_id, static_cast<double>(vertices[i].rank) + ground_add);
        }
    }

    fclose(out);
}
