#pragma once
#include "../Base/Graph.h"
#include "../Base/MetaData.h"

#include <string>
#include <vector>

class MyReader {
public:
    MyReader(std::string path);
    ~MyReader();

    MyReader(const MyReader&) = delete;
    MyReader& operator=(const MyReader&) = delete;

    void ReadBatch(size_t index, std::vector<Edge>& storage);
    void ReadBatchPart(size_t index, size_t edge_offset, size_t count, std::vector<Edge>& storage);
    void ReadEdges(std::vector<Edge>& storage);
    void ReadMeta();
    size_t GetEdgeCount() const;
    size_t GetVertexCount() const;
    const MetaData& GetMetaData() const;
private:
    void ReadRawBytes(size_t offset, char* data, size_t size);

    std::string graph_path_;
    int input_fd_ = -1;
    size_t size_ = 0;
    MetaData meta_;
};
