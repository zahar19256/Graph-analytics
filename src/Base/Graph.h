#pragma once
#include <stdint.h>

struct Edge {
    Edge() = default;
    Edge(int32_t from, int32_t to) : from(from) , to(to) {
    }
    int32_t from , to;
};
