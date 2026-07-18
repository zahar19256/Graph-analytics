#include "GraphConvertor.h"
#include "CsvReader.h"

int64_t HashPair(int32_t i, int32_t j) {
    return i * 1024 + j; 
}

std::pair<int32_t , int32_t> FromHash(int64_t hash) {
    return {hash / 1024 , hash % 1024};
}

bool Compare(const Edge& left , const Edge& right) {
    return left.from < right.from;
}

void Convertor::ToBinaryConvertation(std::string input_file, std::string output_file) {
    CsvReader reader(input_file);
    RawWriter writer(output_file + ".raw");
    std::vector<Edge> edges;
    edges.resize(kEdgeBatchSize);
    while (!reader.Empty()) {
        reader.ReadNext(edges);
        for (const Edge& edge : edges) {
            ++batch_size_[HashPair(edge.to / kFromBatchSize, edge.from / kToBatchSize)];
        }
        writer.Write(edges);
    }
    size_t current_offset = 0;
    for (auto to : batch_size_) {
        int64_t index = to.first;
        size_t size = to.second;
        meta_.InsertInfo(index , {current_offset + size , size});
    }
}

void Convertor::Convertation(std::string input_file, std::string output_file) {
    ToBinaryConvertation(input_file, output_file);
    RawReader reader(output_file + ".raw");
    MmapWriter writer(output_file + ".graphZ" , meta_.GetEdgeCount() * sizeof(int32_t) * 2);
    std::vector<Edge> storage;
    storage.resize(kEdgeBatchSize);
    while(!reader.Empty()) {
        reader.ReadVector<Edge>(kEdgeBatchSize);
        sort(storage.begin() , storage.end() , Compare);
    }
}
