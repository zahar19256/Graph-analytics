#pragma once

struct Edge {
    Edge() = default;
    Edge(int from, int to) : from_(from) , to_(to) {
    }
    int from_ , to_;
};