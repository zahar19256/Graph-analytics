#include "GraphConvertor.h"

void Convertor::ToBinaryConvertation(std::string input_file, std::string output_file) {
    CsvReader reader(input_file);
    RawWriter writer(output_file);
    std::vector<Edge> edges;
    edges.resize(kEdgeBatchSize);
    while (!reader.Empty()) {
        reader.ReadNext(edges);
        for (const Edge& edge : edges) {
            ++batch_size_[{edge.from / kFromBatchSize, edge.to / kToBatchSize}];
        }
        writer.Write(edges);
    }
}
