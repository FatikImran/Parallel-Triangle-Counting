// Name: Muhammad Fatik Bin Imran
// Student ID: 23i-0655
// Section: C
// Assignment: 2 (Triangle Counting)

#include "arch_info.hpp"
#include "triangle_counter.hpp"

#include <chrono>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

namespace {

void print_usage(const char* program) {
    cout << "Usage:\n";
    cout << "  " << program << " <edge_list_file> [threads]\n";
    cout << "\nVariants executed:\n";
    cout << "  1) scalar\n";
    cout << "  2) simd-intrinsics\n";
    cout << "  3) openmp-simd\n";
    cout << "  4) openmp-threads\n";
    cout << "  5) hybrid-intrinsics-openmp\n";
    cout << "  6) hybrid-openmp-simd-openmp\n";
}

void run_variant(const Graph& graph, Variant variant, int threads, uint64_t baseline_triangles, double baseline_time) {
    using clock = chrono::high_resolution_clock;

    // Time one variant end-to-end on the same preprocessed graph.
    auto start = clock::now();
    uint64_t triangles = count_triangles(graph, variant, threads);
    auto end = clock::now();

    double elapsed_ms = chrono::duration<double, milli>(end - start).count();
    double speedup = baseline_time > 0.0 ? (baseline_time / elapsed_ms) : 0.0;

    cout << "Variant: " << variant_to_string(variant) << "\n";
    cout << "  Vertices: " << graph.num_vertices << "\n";
    cout << "  Edges (undirected): " << graph.num_edges_undirected << "\n";
    cout << "  Triangle count: " << triangles << "\n";
    cout << "  Time (ms): " << elapsed_ms << "\n";
    if (variant != Variant::Scalar) {
        cout << "  Speedup vs scalar: " << speedup << "\n";
    }
    if (triangles != baseline_triangles) {
        cout << "  WARNING: triangle count mismatch vs scalar baseline!\n";
    }
    cout << "\n";
}

} // namespace

int main(int argc, char** argv) {
    try {
        // Expect: input edge-list path, optional OpenMP thread count.
        if (argc < 2) {
            print_usage(argv[0]);
            return 1;
        }

        string input_path = argv[1];
        int threads = 0;
        if (argc >= 3) {
            threads = max(1, atoi(argv[2]));
        }

        // Detect architecture and core/thread topology for reporting + defaults.
        ArchInfo arch = detect_arch_info();
        print_arch_info(arch);
        cout << "\n";

        // If not provided by CLI, default to physical cores (or hardware threads fallback).
        if (threads <= 0) {
            threads = arch.physical_cores > 0 ? arch.physical_cores : arch.hardware_threads;
        }

        cout << "Configured OpenMP threads: " << threads << "\n\n";

        // One-time preprocessing shared by all variants for fair comparison.
        Graph graph = load_and_preprocess_graph(input_path);

        using clock = chrono::high_resolution_clock;
        // Compute scalar baseline first: correctness anchor + speedup denominator.
        auto scalar_start = clock::now();
        uint64_t scalar_triangles = count_triangles(graph, Variant::Scalar, threads);
        auto scalar_end = clock::now();
        double scalar_ms = chrono::duration<double, milli>(scalar_end - scalar_start).count();

        cout << "Variant: " << variant_to_string(Variant::Scalar) << "\n";
        cout << "  Vertices: " << graph.num_vertices << "\n";
        cout << "  Edges (undirected): " << graph.num_edges_undirected << "\n";
        cout << "  Triangle count: " << scalar_triangles << "\n";
        cout << "  Time (ms): " << scalar_ms << "\n\n";

        vector<Variant> variants = {
            Variant::SimdIntrinsics,
            Variant::OpenMPSimd,
            Variant::OpenMPThreads,
            Variant::HybridIntrinsicsOpenMP,
            Variant::HybridOpenMPSimdOpenMP,
        };

        // Run all non-scalar variants against the same scalar baseline.
        for (Variant variant : variants) {
            run_variant(graph, variant, threads, scalar_triangles, scalar_ms);
        }

        return 0;
    } catch (const exception& ex) {
        cerr << "Error: " << ex.what() << "\n";
        return 1;
    }
}
