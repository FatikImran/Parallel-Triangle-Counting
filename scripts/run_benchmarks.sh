#!/usr/bin/env bash
# Name: Muhammad Fatik Bin Imran
# Student ID: 23i-0655
# Section: C
# Assignment: 2 (Triangle Counting)

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BIN="$ROOT_DIR/triangle_counter"
DATA_DIR="$ROOT_DIR/datasets"
OUT_DIR="$ROOT_DIR/results"
THREADS="${1:-}"
INCLUDE_LARGE="${2:-0}"

mkdir -p "$OUT_DIR"

if [[ ! -x "$BIN" ]]; then
  echo "Building project..."
  make -C "$ROOT_DIR"
fi

if [[ -z "$THREADS" ]]; then
  physical_cores=""
  if [[ -r /proc/cpuinfo ]]; then
    physical_cores="$(awk -F: '
      BEGIN { phys = ""; core = "" }
      /^physical id/ { gsub(/^[ \t]+|[ \t]+$/, "", $2); phys = $2 }
      /^core id/ { gsub(/^[ \t]+|[ \t]+$/, "", $2); core = $2 }
      /^$/ {
        if (phys != "" && core != "") {
          key = phys ":" core
          seen[key] = 1
        }
        phys = ""; core = ""
      }
      END {
        if (phys != "" && core != "") {
          key = phys ":" core
          seen[key] = 1
        }
        n = 0
        for (k in seen) { n++ }
        print n
      }
    ' /proc/cpuinfo)"
  fi

  if [[ -n "$physical_cores" && "$physical_cores" -gt 0 ]]; then
    THREADS="$physical_cores"
  elif command -v nproc >/dev/null 2>&1; then
    THREADS="$(nproc)"
  else
    THREADS="8"
  fi
fi

TIMESTAMP="$(date +%Y%m%d_%H%M%S)"
CSV="$OUT_DIR/benchmark_${TIMESTAMP}.csv"

echo "dataset,variant,vertices,edges,triangles,time_ms,speedup_vs_scalar,threads" > "$CSV"

DATASETS=(
  "$DATA_DIR/email-Eu-core.txt.gz"
  "$DATA_DIR/facebook_combined.txt.gz"
)

if [[ "$INCLUDE_LARGE" == "1" ]]; then
  DATASETS+=("$DATA_DIR/com-lj.ungraph.txt.gz")
fi

run_one() {
  local dataset_path="$1"
  local dataset_name
  dataset_name="$(basename "$dataset_path")"

  if [[ ! -f "$dataset_path" ]]; then
    echo "Skipping missing dataset: $dataset_name"
    return 0
  fi

  echo "Running: $dataset_name with threads=$THREADS"

  local tmp_file
  tmp_file="$(mktemp)"
  trap 'rm -f "$tmp_file"' RETURN

  gzip -dc "$dataset_path" > "$tmp_file"

  local output
  output="$($BIN "$tmp_file" "$THREADS")"

  awk -v dataset="$dataset_name" -v threads="$THREADS" '
    BEGIN {
      variant=""; vertices=""; edges=""; triangles=""; time_ms=""; speedup="";
    }
    /^Variant:/ {
      if (variant != "") {
        if (speedup == "") speedup = "1.0";
        printf "%s,%s,%s,%s,%s,%s,%s,%s\n", dataset, variant, vertices, edges, triangles, time_ms, speedup, threads;
      }
      variant = $2;
      vertices=""; edges=""; triangles=""; time_ms=""; speedup="";
    }
    /Vertices:/ { vertices = $2; }
    /Edges \(undirected\):/ { edges = $3; }
    /Triangle count:/ { triangles = $3; }
    /Time \(ms\):/ { time_ms = $3; }
    /Speedup vs scalar:/ { speedup = $4; }
    END {
      if (variant != "") {
        if (speedup == "") speedup = "1.0";
        printf "%s,%s,%s,%s,%s,%s,%s,%s\n", dataset, variant, vertices, edges, triangles, time_ms, speedup, threads;
      }
    }
  ' <<< "$output" >> "$CSV"

  rm -f "$tmp_file"
  trap - RETURN
}

for ds in "${DATASETS[@]}"; do
  run_one "$ds"
done

echo "Benchmark complete. CSV written to: $CSV"
echo "Large dataset included: $INCLUDE_LARGE"
