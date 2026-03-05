// Name: Muhammad Fatik Bin Imran
// Student ID: 23i-0655
// Section: C
// Assignment: 2 (Triangle Counting)

#include "arch_info.hpp"

#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <thread>

#if defined(__x86_64__) || defined(__i386__)
#include <cpuid.h>
#endif

using namespace std;

namespace {

int detect_simd_bits_x86() {
#if defined(__x86_64__) || defined(__i386__)
#if defined(__GNUC__)
    // Prefer widest available SIMD extension.
    if (__builtin_cpu_supports("avx512f")) {
        return 512;
    }
    if (__builtin_cpu_supports("avx2")) {
        return 256;
    }
    if (__builtin_cpu_supports("avx")) {
        return 256;
    }
    if (__builtin_cpu_supports("sse2")) {
        return 128;
    }
#endif
#endif
    return 0;
}

string detect_isa() {
#if defined(__aarch64__) || defined(__arm__)
    // ARM SIMD extension.
    return "NEON";
#elif defined(__x86_64__) || defined(__i386__)
    int bits = detect_simd_bits_x86();
    if (bits == 512) {
        return "AVX-512";
    }
    if (bits == 256) {
#if defined(__GNUC__)
        if (__builtin_cpu_supports("avx2")) {
            return "AVX2";
        }
#endif
        return "AVX";
    }
    if (bits == 128) {
        return "SSE2";
    }
    return "SCALAR";
#else
    return "UNKNOWN";
#endif
}

int detect_physical_cores_linux() {
    // Parse /proc/cpuinfo and count unique (physical id, core id) pairs.
    ifstream input("/proc/cpuinfo");
    if (!input) {
        return 0;
    }

    set<pair<int, int>> unique_cores;
    string line;
    int physical_id = -1;
    int core_id = -1;

    auto commit_if_valid = [&]() {
        if (physical_id >= 0 && core_id >= 0) {
            unique_cores.insert({physical_id, core_id});
        }
    };

    while (getline(input, line)) {
        if (line.empty()) {
            commit_if_valid();
            physical_id = -1;
            core_id = -1;
            continue;
        }

        if (line.rfind("physical id", 0) == 0) {
            size_t pos = line.find(':');
            if (pos != string::npos) {
                physical_id = stoi(line.substr(pos + 1));
            }
        } else if (line.rfind("core id", 0) == 0) {
            size_t pos = line.find(':');
            if (pos != string::npos) {
                core_id = stoi(line.substr(pos + 1));
            }
        }
    }
    commit_if_valid();

    if (!unique_cores.empty()) {
        return static_cast<int>(unique_cores.size());
    }
    return 0;
}

} // namespace

ArchInfo detect_arch_info() {
    ArchInfo info;

    // ISA + SIMD width detection.
    info.isa = detect_isa();

    if (info.isa == "AVX-512") {
        info.simd_bits = 512;
    } else if (info.isa == "AVX2" || info.isa == "AVX") {
        info.simd_bits = 256;
    } else if (info.isa == "SSE2" || info.isa == "NEON") {
        info.simd_bits = 128;
    } else {
        info.simd_bits = 0;
    }

    // Lane counts assuming packed float/double vectors.
    info.float_lanes = info.simd_bits > 0 ? info.simd_bits / 32 : 1;
    info.double_lanes = info.simd_bits > 0 ? info.simd_bits / 64 : 1;

    // Thread/core topology for OpenMP configuration.
    info.hardware_threads = static_cast<int>(thread::hardware_concurrency());
    info.physical_cores = detect_physical_cores_linux();
    if (info.physical_cores <= 0) {
        // Fallback when physical-core parsing is unavailable.
        info.physical_cores = info.hardware_threads;
    }

    return info;
}

void print_arch_info(const ArchInfo& info) {
    cout << "Architecture detection:\n";
    cout << "  ISA: " << info.isa << "\n";
    cout << "  SIMD width (bits): " << info.simd_bits << "\n";
    cout << "  Float lanes per vector: " << info.float_lanes << "\n";
    cout << "  Double lanes per vector: " << info.double_lanes << "\n";
    cout << "  Physical cores: " << info.physical_cores << "\n";
    cout << "  Hardware threads: " << info.hardware_threads << "\n";
}
