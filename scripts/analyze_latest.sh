#!/usr/bin/env bash
# Name: Muhammad Fatik Bin Imran
# Student ID: 23i-0655
# Section: C
# Assignment: 2 (Triangle Counting)

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
RESULTS_DIR="$ROOT_DIR/results"

latest_csv="$(ls -1t "$RESULTS_DIR"/benchmark_*.csv 2>/dev/null | head -n 1 || true)"
if [[ -z "$latest_csv" ]]; then
  echo "No benchmark CSV found in $RESULTS_DIR"
  exit 1
fi

echo "Analyzing: $latest_csv"
python3 "$ROOT_DIR/scripts/analyze_results.py" "$latest_csv" --out-dir "$RESULTS_DIR/analysis"

echo "Done. See: $RESULTS_DIR/analysis"
