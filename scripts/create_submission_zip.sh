#!/usr/bin/env bash
# Name: Muhammad Fatik Bin Imran
# Student ID: 23i-0655
# Section: C
# Assignment: 2 (Triangle Counting)

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
STUDENT_FOLDER="23i-0655-C"
STAGING_DIR="$ROOT_DIR/$STUDENT_FOLDER"
ZIP_PATH="$ROOT_DIR/${STUDENT_FOLDER}.zip"
BASE_PREFIX="23i-0655-C"

rm -rf "$STAGING_DIR"
mkdir -p "$STAGING_DIR/src" "$STAGING_DIR/scripts"

MAIN_CPP="$STAGING_DIR/src/${BASE_PREFIX}-TASK1-V1.cpp"
TC_CPP="$STAGING_DIR/src/${BASE_PREFIX}-TASK2-V1.cpp"
TC_HPP="$STAGING_DIR/src/${BASE_PREFIX}-TASK3-V1.hpp"
ARCH_CPP="$STAGING_DIR/src/${BASE_PREFIX}-TASK4-V1.cpp"
ARCH_HPP="$STAGING_DIR/src/${BASE_PREFIX}-TASK5-V1.hpp"

RUN_BENCH="$STAGING_DIR/scripts/${BASE_PREFIX}-TASK6-V1.sh"
ANALYZE_LATEST="$STAGING_DIR/scripts/${BASE_PREFIX}-TASK7-V1.sh"
EXTRACT_DATASETS="$STAGING_DIR/scripts/${BASE_PREFIX}-TASK8-V1.sh"
ANALYZE_RESULTS="$STAGING_DIR/scripts/${BASE_PREFIX}-TASK9-V1.py"

MAIN_CPP_BASENAME="$(basename "$MAIN_CPP")"
TC_CPP_BASENAME="$(basename "$TC_CPP")"
ARCH_CPP_BASENAME="$(basename "$ARCH_CPP")"
TC_HPP_BASENAME="$(basename "$TC_HPP")"
ARCH_HPP_BASENAME="$(basename "$ARCH_HPP")"
ANALYZE_RESULTS_BASENAME="$(basename "$ANALYZE_RESULTS")"

sed \
  -e "s/#include \"arch_info.hpp\"/#include \"$ARCH_HPP_BASENAME\"/" \
  -e "s/#include \"triangle_counter.hpp\"/#include \"$TC_HPP_BASENAME\"/" \
  "$ROOT_DIR/src/main.cpp" > "$MAIN_CPP"

sed \
  -e "s/#include \"triangle_counter.hpp\"/#include \"$TC_HPP_BASENAME\"/" \
  "$ROOT_DIR/src/triangle_counter.cpp" > "$TC_CPP"

cp "$ROOT_DIR/src/triangle_counter.hpp" "$TC_HPP"

sed \
  -e "s/#include \"arch_info.hpp\"/#include \"$ARCH_HPP_BASENAME\"/" \
  "$ROOT_DIR/src/arch_info.cpp" > "$ARCH_CPP"

cp "$ROOT_DIR/src/arch_info.hpp" "$ARCH_HPP"

cp "$ROOT_DIR/scripts/run_benchmarks.sh" "$RUN_BENCH"
sed \
  -e "s#analyze_results.py#$ANALYZE_RESULTS_BASENAME#g" \
  "$ROOT_DIR/scripts/analyze_latest.sh" > "$ANALYZE_LATEST"
cp "$ROOT_DIR/scripts/extract_datasets.sh" "$EXTRACT_DATASETS"
cp "$ROOT_DIR/scripts/analyze_results.py" "$ANALYZE_RESULTS"

cat > "$STAGING_DIR/Makefile" <<EOF
CXX := g++
CXXFLAGS := -O3 -std=c++17 -Wall -Wextra -march=native -fopenmp
LDFLAGS := -fopenmp

SRC := src/$MAIN_CPP_BASENAME src/$TC_CPP_BASENAME src/$ARCH_CPP_BASENAME
OBJ := \
  src/${MAIN_CPP_BASENAME%.cpp}.o \
  src/${TC_CPP_BASENAME%.cpp}.o \
  src/${ARCH_CPP_BASENAME%.cpp}.o
TARGET := triangle_counter

all: \$(TARGET)

\$(TARGET): \$(OBJ)
	\$(CXX) \$(OBJ) -o \$@ \$(LDFLAGS)

src/%.o: src/%.cpp
	\$(CXX) \$(CXXFLAGS) -c \$< -o \$@

clean:
	rm -f \$(OBJ) \$(TARGET)

.PHONY: all clean
EOF

chmod +x "$RUN_BENCH" "$ANALYZE_LATEST" "$EXTRACT_DATASETS"

if [[ -f "$ROOT_DIR/REPORT.pdf" ]]; then
  cp "$ROOT_DIR/REPORT.pdf" "$STAGING_DIR/"
else
  echo "Warning: REPORT.pdf not found. Assignment instructions require a PDF report."
fi

rm -f "$ZIP_PATH"
if command -v zip >/dev/null 2>&1; then
  (
    cd "$ROOT_DIR"
    zip -r "${STUDENT_FOLDER}.zip" "$STUDENT_FOLDER" >/dev/null
  )
else
  python3 - <<PY
import os
import zipfile

root = r"$ROOT_DIR"
folder = r"$STUDENT_FOLDER"
zip_path = r"$ZIP_PATH"
folder_path = os.path.join(root, folder)

with zipfile.ZipFile(zip_path, "w", compression=zipfile.ZIP_DEFLATED) as zf:
    for base, _, files in os.walk(folder_path):
        for name in files:
            abs_path = os.path.join(base, name)
            rel_path = os.path.relpath(abs_path, root)
            zf.write(abs_path, rel_path)
PY
fi

echo "Created: $ZIP_PATH"
echo "Staging folder: $STAGING_DIR"
echo "Contents policy: staged folder includes code files and REPORT.pdf (if present)."
