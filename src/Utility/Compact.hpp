#pragma once

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

#include "../Base/Graph.h"
#include "../IO/RawReader.hpp"
#include "../IO/RawWriter.hpp"
#include "Sort.hpp"

// данный файлик полностью создан через ллм я не успевал по дд :(
// у меня на последних этапах реализации было осознанно, что из-за пробелов в номерах вершин 
// место с хранением встепеней и рангов вершин становится огромным буквально sizeof(VertexInfo) * max_node
// что сильно больше, чем реально нужно

// тут происходит сжатие рёбер я использую алгос:
/*
1) отсортить и убрать повторения среди всех вершин
2) считаем порядковые номера для всех вершин делаем mapping
3) дальше мы заменяем номера вершин в рёбрах на сжатые плотные версии для этого:
делаем сорт вершин сначал по from и замену номеров у from, потом сорт по to и аналогично замена номеров по to
Такой алгоритм уплотняет рёбра и решает проблему следующего характера:
при первом запуске своего алгоритма без внешнего сорта и данной сжатия я получал файл с VertexInfo размером 12.4 ГБ 
на дата сете: https://snap.stanford.edu/data/ego-Twitter.html 
из-за чего каждая итерация алгоритма длилась от 70 до 100 секунд, так как было много переходов по почти пустому файлу вершин
*/


struct VertexMapping {
    int32_t old_id;
    int32_t new_id;
};

struct HalfMappedEdge {
    int32_t new_from;
    int32_t old_to;
};

class EdgeFromCompare {
public:
    bool operator()(const Edge& left, const Edge& right) const {
        if (left.from != right.from) {
            return left.from < right.from;
        }
        return left.to < right.to;
    }
};

class HalfToCompare {
public:
    bool operator()(const HalfMappedEdge& left, const HalfMappedEdge& right) const {
        if (left.old_to != right.old_to) {
            return left.old_to < right.old_to;
        }
        return left.new_from < right.new_from;
    }
};

static void BuildMapping(const std::string& file) {
    RawReader reader(file + ".sort");
    RawWriter writer(file + ".mapping");
    std::vector<int32_t> ids;
    std::vector<VertexMapping> mapping;
    int32_t next_id = 0;

    while(!reader.Empty()) {
        reader.ReadVector<int32_t>(ids, kSortBatchSize);
        mapping.resize(ids.size());
        for (size_t i = 0; i < ids.size(); ++i) {
            mapping[i] = {ids[i], next_id};
            ++next_id;
        }
        writer.Write(mapping);
    }
    writer.Flush();
}

static void SortEdgesByFrom(const std::string& file) {
    RawReader reader(file + ".raw");
    SortFile<Edge, EdgeFromCompare> sort_file(file + ".edges_from", 0, EdgeFromCompare(), false);
    std::vector<Edge> edges;

    while(!reader.Empty()) {
        reader.ReadVector<Edge>(edges, kSortBatchSize);
        std::sort(edges.begin(), edges.end(), EdgeFromCompare());
        sort_file.AddBatch(edges);
    }

    sort_file.Finish(file + ".edges_from.sort");
}

static void RemapFrom(const std::string& file) {
    RawReader edge_reader(file + ".edges_from.sort");
    RawReader mapping_reader(file + ".mapping");
    RawWriter writer(file + ".half");

    VertexMapping mapping{};
    mapping_reader.Read(mapping);

    std::vector<Edge> edges;
    std::vector<HalfMappedEdge> half;
    while(!edge_reader.Empty()) {
        edge_reader.ReadVector<Edge>(edges, kSortBatchSize);
        half.clear();
        half.reserve(edges.size());
        for (Edge& edge : edges) {
            while (mapping.old_id < edge.from) {
                mapping_reader.Read(mapping);
            }
            half.push_back({mapping.new_id, edge.to});
        }
        writer.Write(half);
    }
    writer.Flush();
}

static void SortHalfByTo(const std::string& file) {
    RawReader reader(file + ".half");
    SortFile<HalfMappedEdge, HalfToCompare> sort_file(file + ".half_to", 0, HalfToCompare(), false);
    std::vector<HalfMappedEdge> edges;

    while(!reader.Empty()) {
        reader.ReadVector<HalfMappedEdge>(edges, kSortBatchSize);
        std::sort(edges.begin(), edges.end(), HalfToCompare());
        sort_file.AddBatch(edges);
    }

    sort_file.Finish(file + ".half_to.sort");
}

static void RemapTo(const std::string& file) {
    RawReader edge_reader(file + ".half_to.sort");
    RawReader mapping_reader(file + ".mapping");
    RawWriter writer(file + ".compact.raw");

    VertexMapping mapping{};
    mapping_reader.Read(mapping);

    std::vector<HalfMappedEdge> half;
    std::vector<Edge> edges;
    while(!edge_reader.Empty()) {
        edge_reader.ReadVector<HalfMappedEdge>(half, kSortBatchSize);
        edges.clear();
        edges.reserve(half.size());
        for (HalfMappedEdge& edge : half) {
            while (mapping.old_id < edge.old_to) {
                mapping_reader.Read(mapping);
            }
            edges.push_back({edge.new_from, mapping.new_id});
        }
        writer.Write(edges);
    }
    writer.Flush();
}

static void Compact(const std::string& file) {
    Sort(file);
    BuildMapping(file);
    SortEdgesByFrom(file);
    RemapFrom(file);
    SortHalfByTo(file);
    RemapTo(file);
}
