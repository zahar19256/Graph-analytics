#include "GraphConvertor.h"
#include "CsvReader.h"
#include "FileFormat.h"
#include "../Utility/Compact.hpp"

#include <chrono>
#include <cstring>
#include <cstdint>
#include <fstream>
// #include <iostream>
#include <stdexcept>

struct LocalBatchInfo {
    size_t offset;
    size_t size;
};

int64_t HashPair(int32_t i, int32_t j) {
    return i * kMaxBucketCount + j;
}

int64_t GetIndex(const Edge& edge) {
    return HashPair(edge.to / kToBatchSize , edge.from / kFromBatchSize);
}

std::pair<int32_t , int32_t> FromHash(int64_t hash) {
    return {hash / kMaxBucketCount , hash % kMaxBucketCount};
}

bool Compare(const Edge& left , const Edge& right) {
    if (left.to / kToBatchSize != right.to / kToBatchSize) {
        return left.to < right.to;
    } else {
        return left.from < right.from;
    }
}

void Convertor::ToBinaryConvertation(const std::string& input_file, const std::string& output_file) {
    batch_size_.clear();
    meta_.Clear();
    CsvReader reader(input_file);
    RawWriter writer(output_file + ".raw");
    std::vector<Edge> edges;
    edges.resize(kEdgeBatchSize);
    while (!reader.Empty()) {
        reader.ReadNext(edges);
        for (Edge& edge : edges) {
            if (edge.from < 0 || edge.to < 0) {
                throw std::runtime_error("Negative vertex id is not supported!");
            }
            ++batch_size_[GetIndex(edge)];
            meta_.TouchVertex(static_cast<size_t>(edge.from));
            meta_.TouchVertex(static_cast<size_t>(edge.to));
        }
        writer.Write(edges);
    }
    size_t current_offset = 0;
    for (std::pair<const int64_t, int64_t> to : batch_size_) {
        int64_t index = to.first;
        size_t size = to.second;
        meta_.InsertInfo(index , {current_offset , size , 0});
        current_offset += size;
    }
}

void Convertor::BuildMetaFromRaw(const std::string& raw_file) {
    batch_size_.clear();
    meta_.Clear();
    RawReader reader(raw_file);
    std::vector<Edge> edges;
    edges.resize(kEdgeBatchSize);
    while (!reader.Empty()) {
        reader.ReadVector<Edge>(edges, kEdgeBatchSize);
        for (Edge& edge : edges) {
            ++batch_size_[GetIndex(edge)];
            meta_.TouchVertex(static_cast<size_t>(edge.from));
            meta_.TouchVertex(static_cast<size_t>(edge.to));
        }
    }
    size_t current_offset = 0;
    for (std::pair<const int64_t, int64_t>& to : batch_size_) {
        int64_t index = to.first;
        size_t size = to.second;
        meta_.InsertInfo(index , {current_offset , size , 0});
        current_offset += size;
    }
}

void Convertor::WriteEdges(const std::vector<Edge>& edges, MmapWriter& writer) {
    size_t start = 0;
    while (start < edges.size()) {
        int64_t index = GetIndex(edges[start]);
        size_t last = start + 1;
        while (last < edges.size() && GetIndex(edges[last]) == index) {
            ++last;
        }
        size_t count = last - start;
        size_t edge_offset = meta_.GetOffset(index) + meta_.GetUsedSize(index);
        writer.Write(edge_offset * sizeof(Edge), edges.data() + start, count);
        meta_.FillBatch(index, count);
        start = last;
    }
}

void Convertor::WriteMeta(MmapWriter& writer, size_t meta_offset) const {
    size_t offset = meta_offset;
    GraphMetaHeader header{
        static_cast<uint64_t>(meta_.GetEdgeCount()),
        static_cast<uint64_t>(meta_.GetBatches().size()),
        static_cast<uint64_t>(meta_.GetVertexCount()),
        static_cast<uint64_t>(kFromBatchSize),
        static_cast<uint64_t>(kToBatchSize)
    };
    for (const std::pair<const int64_t, batch_info>& item : meta_.GetBatches()) {
        const batch_info& info = item.second;
        GraphMetaBatchRecord record{
            item.first,
            static_cast<uint64_t>(info.offset),
            static_cast<uint64_t>(info.size)
        };
        writer.Write(offset, record);
        offset += sizeof(record);
    }
    writer.Write(offset, header);
}

size_t Convertor::GetMetaSize() const {
    size_t batch_count = meta_.GetBatches().size();
    size_t batch_bytes = batch_count * sizeof(GraphMetaBatchRecord);
    return batch_bytes + sizeof(GraphMetaHeader);
}

void Convertor::SetupVertexFile(const std::string& output_file) {
    size_t vertex_count = meta_.GetVertexCount();
    size_t max_from_batch = (vertex_count + kFromBatchSize - 1) / kFromBatchSize;
    std::vector<std::vector<LocalBatchInfo>> column_batches(max_from_batch);
    std::vector<int64_t> local_degrees(kFromBatchSize, 0);
    std::vector<VertexInfo> output_buffer(kFromBatchSize);
    std::vector<Edge> edge_storage;
    for (const std::pair<const int64_t, batch_info>& item : meta_.GetBatches()) {
        int64_t index = item.first;
        const batch_info& info = item.second;
        size_t from_batch = index % kMaxBucketCount;
        if (from_batch < max_from_batch && info.size > 0) {
            column_batches[from_batch].push_back({info.offset, info.size});
        }
    }
    std::ofstream vertex_out(output_file + ".vertex_info", std::ios::binary | std::ios::trunc);
    if (!vertex_out) {
        throw std::runtime_error("Failed to open .vertex_info for writing!");
    }
    RawReader mapping_reader(output_file + ".mapping");
    RawReader graph_reader(output_file + ".graphZ");
    for (size_t from_batch = 0; from_batch < max_from_batch; ++from_batch) {
        fill(local_degrees.begin(), local_degrees.end(), 0);
        for (const LocalBatchInfo& batch : column_batches[from_batch]) {
            size_t left = batch.size;
            size_t edge_offset = batch.offset;
            while (left > 0) {
                size_t count = std::min(kEdgeReadBatchSize, left);
                graph_reader.Seek(edge_offset * sizeof(Edge));
                graph_reader.ReadVector<Edge>(edge_storage, count);
                for (Edge& edge : edge_storage) {
                    int32_t local_from = edge.from % kFromBatchSize;
                    local_degrees[local_from]++;
                }
                edge_offset += edge_storage.size();
                left -= edge_storage.size();
            }
        }
        size_t current_batch_vertices = kFromBatchSize;
        if ((from_batch + 1) * kFromBatchSize > vertex_count) {
            current_batch_vertices = vertex_count - from_batch * kFromBatchSize;
        }
        for (size_t i = 0; i < current_batch_vertices; ++i) {
            VertexMapping mapping{};
            mapping_reader.Read(mapping);
            output_buffer[i].old_id = mapping.old_id;
            output_buffer[i].out_degree = local_degrees[i];
            output_buffer[i].rank = 1.0;
            output_buffer[i].next_rank = 0.0;
        }
        vertex_out.write(reinterpret_cast<const char*>(output_buffer.data()), current_batch_vertices * sizeof(VertexInfo));
    }
}

void Convertor::Convertation(const std::string& input_file, const std::string& output_file) {
    std::chrono::steady_clock::time_point start_time = std::chrono::steady_clock::now();
    ToBinaryConvertation(input_file, output_file);
    Compact(output_file);
    BuildMetaFromRaw(output_file + ".compact.raw");

    RawReader reader(output_file + ".compact.raw");
    size_t edge_bytes = meta_.GetEdgeCount() * sizeof(Edge);
    size_t meta_bytes = GetMetaSize();
    MmapWriter writer(output_file + ".graphZ" , edge_bytes + meta_bytes);
    std::vector<Edge> storage;
    storage.resize(kEdgeBatchSize);
    while(!reader.Empty()) {
        reader.ReadVector<Edge>(storage, kEdgeBatchSize);
        sort(storage.begin() , storage.end() , Compare);
        WriteEdges(storage, writer);
    }
    writer.Flush();
    WriteMeta(writer, edge_bytes);
    writer.Flush();
    SetupVertexFile(output_file);
    std::chrono::steady_clock::time_point finish_time = std::chrono::steady_clock::now();
    double seconds = std::chrono::duration<double>(finish_time - start_time).count();
    //std::cerr << "convert time: " << seconds << " sec" << std::endl;
}
