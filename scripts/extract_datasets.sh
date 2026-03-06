#!/usr/bin/env bash
# Name: Muhammad Fatik Bin Imran
# Student ID: 23i-0655
# Section: C
# Assignment: 2 (Triangle Counting)

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
DATA_DIR="$ROOT_DIR/datasets"
EXTRACTED_DIR="$ROOT_DIR/datasets/extracted"

mkdir -p "$EXTRACTED_DIR"

for gz in "$DATA_DIR"/*.txt.gz; do
  [[ -f "$gz" ]] || continue
  name="$(basename "$gz" .gz)"
  out="$EXTRACTED_DIR/$name"
  if [[ -f "$out" ]]; then
    echo "Exists: $out"
  else
    echo "Extracting: $gz -> $out"
    gzip -dc "$gz" > "$out"
  fi
done

echo "Done. Extracted files are in: $EXTRACTED_DIR"
