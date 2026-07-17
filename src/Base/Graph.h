#pragma once

struct Edge {
    Edge() = default;
    Edge(int from, int to) : from(from) , to(to) {
    }
    int from , to;
};
