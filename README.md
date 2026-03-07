## Quick Start (3 commands)

```bash
make
./scripts/run_benchmarks.sh 8 1
./scripts/analyze_latest.sh
```

This builds the project, runs all main datasets (including LiveJournal), and generates analysis artifacts under `results/analysis/`.

## Build

```bash
make
```

## Run

```bash
./triangle_counter <edge_list_file> [threads]
```

Examples:

```bash
./triangle_counter data/sanity_edges.txt
./triangle_counter data/sanity_edges.txt 8
```

## Project structure (what each file does)

### Core C++ code

- `src/main.cpp`
	- Program entry point.
	- Parses CLI args (`input_path`, optional `threads`).
	- Prints architecture info, runs scalar baseline, then runs all optimized variants.
	- Reports vertices, edges, triangle count, time, and speedup.

- `src/triangle_counter.hpp`
	- Public API and shared data structures.
	- Declares `Graph`, `Variant`, preprocessing and counting function signatures.

- `src/triangle_counter.cpp`
	- Graph loading and preprocessing:
		- reads edge list,
		- removes self-loops,
		- removes duplicate undirected edges,
		- builds degree-based ordering,
		- builds oriented forward adjacency.
	- Triangle counting kernels:
		- scalar,
		- SIMD intrinsics,
		- OpenMP SIMD,
		- OpenMP threading,
		- hybrid variants.

- `src/arch_info.hpp`
	- Architecture info struct and function declarations.

- `src/arch_info.cpp`
	- Detects ISA/SIMD capability and machine core/thread topology.
	- Prints architecture details used in experiments/report.

- `Makefile`
	- Builds the `triangle_counter` executable with OpenMP enabled.

### Scripts and analysis

- `scripts/run_benchmarks.sh`
	- Runs datasets automatically.
	- Decompresses `.txt.gz` datasets to temp text files.
	- Executes all variants and writes benchmark CSV under `results/`.

- `scripts/analyze_latest.sh`
	- Finds latest benchmark CSV and runs analysis.

- `scripts/analyze_results.py`
	- Aggregates benchmark rows, computes best variant per dataset,
		writes `aggregated_results.csv` and `summary.md`, and optionally plots PNGs.

- `scripts/extract_datasets.sh`
	- One-time extraction helper for all `.txt.gz` datasets.

- `scripts/create_submission_zip.sh`
	- Creates submission staging folder/zip with strict assignment naming.

### Data and outputs

- `data/`
	- Small sanity input(s) for quick correctness testing.

- `datasets/`
	- Assignment datasets (`.txt.gz` compressed edge lists).

- `results/`
	- Raw benchmark CSV outputs and analysis artifacts.

## Common input mistakes

- `triangle_counter` expects a **plain text edge list** file as input.
- Passing `.tar.gz` or `.gz` directly to `triangle_counter` can produce invalid parsing or meaningless output.
- Correct usage options:
	- Decompress first, then pass the extracted `.txt` file to `triangle_counter`.
	- Or use `scripts/run_benchmarks.sh`, which handles `.txt.gz` decompression automatically.

## Datasets in this workspace

- `datasets/email-Eu-core.txt.gz`
- `datasets/facebook_combined.txt.gz`
- `datasets/com-lj.ungraph.txt.gz`

(`com-friendster.ungraph.txt.gz` is intentionally not included due to size.)

## Benchmark automation

Run all available assignment datasets and export a CSV:

```bash
./scripts/run_benchmarks.sh [threads] [include_large]
```

- `threads`: optional (default prefers detected physical cores; falls back to `nproc`)
- `include_large`: optional `0/1`; when `1`, also runs `com-lj.ungraph.txt.gz`

The output CSV is written under `results/`.

Optional one-time extraction of all `.txt.gz` files:

```bash
./scripts/extract_datasets.sh
```

## Analyze benchmark results

Analyze the latest benchmark CSV and generate report-ready artifacts:

```bash
./scripts/analyze_latest.sh
```

Or analyze a specific CSV:

```bash
python3 scripts/analyze_results.py results/benchmark_YYYYMMDD_HHMMSS.csv --out-dir results/analysis
```

Generated outputs:

- `results/analysis/aggregated_results.csv`
- `results/analysis/summary.md`
- per-dataset PNG plots (if `matplotlib` is installed)

## Input format

- Plain text edge list with two vertex IDs per line.
- Lines starting with `#` or `%` are treated as comments.
- Self-loops are removed.
- Duplicate undirected edges are removed.

## Variants implemented

1. `scalar`
2. `simd-intrinsics`
3. `openmp-simd`
4. `openmp-threads`
5. `hybrid-intrinsics-openmp`
6. `hybrid-openmp-simd-openmp`

## Notes

- Vertex ordering is degree-based (tie-break by vertex ID).
- Triangle counting uses oriented forward adjacency and counts each triangle exactly once.
- OpenMP thread count is controlled by CLI argument; if omitted, defaults to detected physical cores.

## Requirement mapping

- **Architecture awareness**: ISA/SIMD width, vector lanes, physical cores, and hardware threads are printed at startup.
- **Input preprocessing correctness**: self-loops and duplicate undirected edges are removed.
- **Duplicate triangle avoidance**: degree-based vertex ordering with forward-oriented adjacency is used.
- **Variant coverage**: scalar baseline + all 5 required comparison variants are executed from one binary.
- **Thread control**: all OpenMP variants use explicit `omp_set_num_threads`; benchmark script defaults to physical cores when not specified.

## Deliverables in this repo

- Source code and build system: `src/`, `Makefile`
- Reproducibility scripts: `scripts/run_benchmarks.sh`, `scripts/analyze_latest.sh`, `scripts/analyze_results.py`
- Current experimental outputs: `results/` and `results/analysis/`
