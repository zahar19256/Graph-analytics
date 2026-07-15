#include "CsvReader.h"
#include <memory.h>

int32_t ParseInt32(const char* start , const char* end) {
    int32_t result = 0;
    while (start < end && *start >= '0' && *start <= '9') {
        result = result * 10 + (*start - '0');
        ++start;
    }
    return result;
}

void CsvReader::ReadNext(std::vector<Edge>& storage) {
    if (storage.capacity() < kEdgeBatchSize) {
        storage.resize(kEdgeBatchSize);
    }
    storage.clear();
    size_t count = 0;
    const char* start = reinterpret_cast<char*>(ptr_) + 7;
    const char* end = start + size_;
    while (count < kEdgeBatchSize && start < end) {
        int32_t from = ParseInt32(start , end);
        ++start;
        int32_t to = ParseInt32(start , end);
        ++start;
        storage[count++] = {from , to};
    }
    offset_ = start - reinterpret_cast<char*>(ptr_);
}
