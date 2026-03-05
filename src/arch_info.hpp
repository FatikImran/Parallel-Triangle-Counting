// Name: Muhammad Fatik Bin Imran
// Student ID: 23i-0655
// Section: C
// Assignment: 2 (Triangle Counting)

#pragma once

#include <string>

struct ArchInfo {
    std::string isa;
    int simd_bits = 0;
    int float_lanes = 0;
    int double_lanes = 0;
    int physical_cores = 0;
    int hardware_threads = 0;
};

ArchInfo detect_arch_info();
void print_arch_info(const ArchInfo& info);
