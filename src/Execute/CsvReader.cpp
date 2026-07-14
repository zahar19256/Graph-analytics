#include "CsvReader.h"
#include <memory.h>

void CsvReader::ReadNext(std::vector<Edge>& storage) {
    if (storage.capacity() < 8192) {
        storage.reserve(8192);
    }
    storage.clear();
    size_t count = 0;
    char* start = reinterpret_cast<char*>(ptr_);
    while (count < 8192 && offset < size_) {
        char* next = static_cast<char*>(memchr(start + offset , ',' , size_ - offset));
        if (next == nullptr) {
            
        }
    }
}