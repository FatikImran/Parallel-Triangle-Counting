// Name: Muhammad Fatik Bin Imran
// Student ID: 23i-0655
// Section: C
// Assignment: 2 (Triangle Counting)

#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

struct Graph {
    std::size_t num_vertices = 0;
    std::size_t num_edges_undirected = 0;
    std::vector<std::vector<int>> forward_adj;
};

enum class Variant {
    Scalar,
    SimdIntrinsics,
    OpenMPSimd,
    OpenMPThreads,
    HybridIntrinsicsOpenMP,
    HybridOpenMPSimdOpenMP
};

Graph load_and_preprocess_graph(const std::string& path);

std::uint64_t count_triangles(const Graph& graph, Variant variant, int num_threads);

const char* variant_to_string(Variant variant);
