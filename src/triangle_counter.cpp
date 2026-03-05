// Name: Muhammad Fatik Bin Imran
// Student ID: 23i-0655
// Section: C
// Assignment: 2 (Triangle Counting)

#include "triangle_counter.hpp"

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

#include <omp.h>

#if defined(__AVX2__)
#include <immintrin.h>
#endif

using namespace std;

namespace {

struct Edge {
    int u;
    int v;
};

vector<Edge> read_edge_list(const string& path, unordered_map<long long, int>& remap) {
    ifstream input(path);
    if (!input) {
        throw runtime_error("Failed to open input file: " + path);
    }

    vector<Edge> edges;
    string line;
    remap.reserve(1 << 20);

    // Read edge-list lines, skipping comments and malformed rows.
    while (getline(input, line)) {
        if (line.empty() || line[0] == '#' || line[0] == '%') {
            continue;
        }

        istringstream iss(line);
        long long a_raw = 0;
        long long b_raw = 0;
        if (!(iss >> a_raw >> b_raw)) {
            continue;
        }

        // Remap original vertex IDs (which can be sparse/large) to dense [0..n-1].
        auto get_or_assign = [&](long long value) {
            auto it = remap.find(value);
            if (it != remap.end()) {
                return it->second;
            }
            int id = static_cast<int>(remap.size());
            remap.emplace(value, id);
            return id;
        };

        int a = get_or_assign(a_raw);
        int b = get_or_assign(b_raw);
        // Remove self-loops: (u, u) cannot contribute to a triangle in a simple graph.
        if (a == b) {
            continue;
        }

        // Canonicalize undirected edge as (min, max) so duplicates can be removed reliably.
        if (a > b) {
            swap(a, b);
        }
        edges.push_back({a, b});
    }

    // Sort + unique removes duplicate undirected edges after canonicalization.
    sort(edges.begin(), edges.end(), [](const Edge& left, const Edge& right) {
        if (left.u != right.u) {
            return left.u < right.u;
        }
        return left.v < right.v;
    });
    edges.erase(unique(edges.begin(), edges.end(), [](const Edge& left, const Edge& right) {
        return left.u == right.u && left.v == right.v;
    }), edges.end());

    return edges;
}

vector<vector<int>> build_undirected_adj(size_t n, const vector<Edge>& edges) {
    vector<vector<int>> adj(n);
    // Build symmetric adjacency lists for an undirected graph.
    for (const auto& e : edges) {
        adj[e.u].push_back(e.v);
        adj[e.v].push_back(e.u);
    }

    for (auto& nbrs : adj) {
        sort(nbrs.begin(), nbrs.end());
    }
    return adj;
}

vector<int> build_vertex_order(const vector<vector<int>>& adj) {
    vector<int> vertices(adj.size());
    iota(vertices.begin(), vertices.end(), 0);

    // Degree-based ordering (ascending), tie-broken by vertex id.
    sort(vertices.begin(), vertices.end(), [&](int a, int b) {
        if (adj[a].size() != adj[b].size()) {
            return adj[a].size() < adj[b].size();
        }
        return a < b;
    });

    // rank[v] gives the position of vertex v in the degree-sorted order.
    vector<int> rank(adj.size());
    for (size_t i = 0; i < vertices.size(); ++i) {
        rank[vertices[i]] = static_cast<int>(i);
    }
    return rank;
}

vector<vector<int>> build_forward_adj(const vector<vector<int>>& adj, const vector<int>& rank) {
    vector<vector<int>> forward(adj.size());

    // Orient each undirected edge from lower-rank to higher-rank endpoint.
    // This guarantees each triangle is counted exactly once.
    for (int u = 0; u < static_cast<int>(adj.size()); ++u) {
        for (int v : adj[u]) {
            if (rank[u] < rank[v]) {
                forward[u].push_back(v);
            }
        }
        sort(forward[u].begin(), forward[u].end());
    }

    return forward;
}

inline uint64_t intersect_scalar(const vector<int>& a, const vector<int>& b) {
    size_t i = 0;
    size_t j = 0;
    uint64_t count = 0;

    // Standard two-pointer intersection on sorted arrays.
    while (i < a.size() && j < b.size()) {
        if (a[i] == b[j]) {
            ++count;
            ++i;
            ++j;
        } else if (a[i] < b[j]) {
            ++i;
        } else {
            ++j;
        }
    }

    return count;
}

#if defined(__AVX2__)
inline bool avx2_contains_value(const int* data, size_t len, int value) {
    __m256i target = _mm256_set1_epi32(value);
    size_t i = 0;

    // Compare one target value against 8 integers at a time.
    for (; i + 8 <= len; i += 8) {
        __m256i chunk = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(data + i));
        __m256i cmp = _mm256_cmpeq_epi32(chunk, target);
        if (_mm256_movemask_epi8(cmp) != 0) {
            return true;
        }
    }

    for (; i < len; ++i) {
        if (data[i] == value) {
            return true;
        }
    }
    return false;
}
#endif

inline uint64_t intersect_simd_intrinsics(const vector<int>& left, const vector<int>& right) {
#if defined(__AVX2__)
    // Probe the smaller list against the larger list to reduce total comparisons.
    const vector<int>& small = (left.size() <= right.size()) ? left : right;
    const vector<int>& large = (left.size() <= right.size()) ? right : left;

    uint64_t count = 0;
    const int* large_data = large.data();
    const size_t large_len = large.size();
    for (int value : small) {
        if (avx2_contains_value(large_data, large_len, value)) {
            ++count;
        }
    }
    return count;
#else
    return intersect_scalar(left, right);
#endif
}

inline uint64_t intersect_omp_simd(const vector<int>& left, const vector<int>& right) {
    const vector<int>& small = (left.size() <= right.size()) ? left : right;
    const vector<int>& large = (left.size() <= right.size()) ? right : left;

    uint64_t count = 0;
// Ask the compiler/OpenMP runtime to vectorize this loop.
#pragma omp simd reduction(+ : count)
    for (size_t idx = 0; idx < small.size(); ++idx) {
        count += static_cast<uint64_t>(binary_search(large.begin(), large.end(), small[idx]));
    }
    return count;
}

uint64_t count_scalar(const Graph& graph) {
    uint64_t triangles = 0;
    // For every oriented edge (u, v), count common forward neighbors.
    for (size_t u = 0; u < graph.forward_adj.size(); ++u) {
        for (int v : graph.forward_adj[u]) {
            triangles += intersect_scalar(graph.forward_adj[u], graph.forward_adj[v]);
        }
    }
    return triangles;
}

uint64_t count_simd_intrinsics(const Graph& graph) {
    uint64_t triangles = 0;
    for (size_t u = 0; u < graph.forward_adj.size(); ++u) {
        for (int v : graph.forward_adj[u]) {
            triangles += intersect_simd_intrinsics(graph.forward_adj[u], graph.forward_adj[v]);
        }
    }
    return triangles;
}

uint64_t count_openmp_simd(const Graph& graph) {
    uint64_t triangles = 0;
    for (size_t u = 0; u < graph.forward_adj.size(); ++u) {
        for (int v : graph.forward_adj[u]) {
            triangles += intersect_omp_simd(graph.forward_adj[u], graph.forward_adj[v]);
        }
    }
    return triangles;
}

uint64_t count_openmp_threads(const Graph& graph) {
    uint64_t triangles = 0;
// Parallelize vertices; dynamic scheduling helps with skewed degree distributions.
#pragma omp parallel for reduction(+ : triangles) schedule(dynamic, 32)
    for (int u = 0; u < static_cast<int>(graph.forward_adj.size()); ++u) {
        uint64_t local = 0;
        for (int v : graph.forward_adj[u]) {
            local += intersect_scalar(graph.forward_adj[u], graph.forward_adj[v]);
        }
        triangles += local;
    }
    return triangles;
}

uint64_t count_hybrid_intrinsics_openmp(const Graph& graph) {
    uint64_t triangles = 0;
// Hybrid: OpenMP on outer loop + AVX2 intrinsics in intersection kernel.
#pragma omp parallel for reduction(+ : triangles) schedule(dynamic, 32)
    for (int u = 0; u < static_cast<int>(graph.forward_adj.size()); ++u) {
        uint64_t local = 0;
        for (int v : graph.forward_adj[u]) {
            local += intersect_simd_intrinsics(graph.forward_adj[u], graph.forward_adj[v]);
        }
        triangles += local;
    }
    return triangles;
}

uint64_t count_hybrid_omp_simd_openmp(const Graph& graph) {
    uint64_t triangles = 0;
// Hybrid: OpenMP on outer loop + OpenMP SIMD in intersection kernel.
#pragma omp parallel for reduction(+ : triangles) schedule(dynamic, 32)
    for (int u = 0; u < static_cast<int>(graph.forward_adj.size()); ++u) {
        uint64_t local = 0;
        for (int v : graph.forward_adj[u]) {
            local += intersect_omp_simd(graph.forward_adj[u], graph.forward_adj[v]);
        }
        triangles += local;
    }
    return triangles;
}

} // namespace

Graph load_and_preprocess_graph(const string& path) {
    unordered_map<long long, int> remap;
    vector<Edge> edges = read_edge_list(path, remap);

    Graph graph;
    graph.num_vertices = remap.size();
    graph.num_edges_undirected = edges.size();

    // Preprocessing pipeline: undirected adjacency -> vertex ordering -> forward adjacency.
    auto undirected_adj = build_undirected_adj(graph.num_vertices, edges);
    auto rank = build_vertex_order(undirected_adj);
    graph.forward_adj = build_forward_adj(undirected_adj, rank);

    return graph;
}

uint64_t count_triangles(const Graph& graph, Variant variant, int num_threads) {
    if (num_threads > 0) {
        omp_set_num_threads(num_threads);
    }

    switch (variant) {
    case Variant::Scalar:
        return count_scalar(graph);
    case Variant::SimdIntrinsics:
        return count_simd_intrinsics(graph);
    case Variant::OpenMPSimd:
        return count_openmp_simd(graph);
    case Variant::OpenMPThreads:
        return count_openmp_threads(graph);
    case Variant::HybridIntrinsicsOpenMP:
        return count_hybrid_intrinsics_openmp(graph);
    case Variant::HybridOpenMPSimdOpenMP:
        return count_hybrid_omp_simd_openmp(graph);
    default:
        throw runtime_error("Unknown variant selected");
    }
}

const char* variant_to_string(Variant variant) {
    switch (variant) {
    case Variant::Scalar:
        return "scalar";
    case Variant::SimdIntrinsics:
        return "simd-intrinsics";
    case Variant::OpenMPSimd:
        return "openmp-simd";
    case Variant::OpenMPThreads:
        return "openmp-threads";
    case Variant::HybridIntrinsicsOpenMP:
        return "hybrid-intrinsics-openmp";
    case Variant::HybridOpenMPSimdOpenMP:
        return "hybrid-openmp-simd-openmp";
    default:
        return "unknown";
    }
}
