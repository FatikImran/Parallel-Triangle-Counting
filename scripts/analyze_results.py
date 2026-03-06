#!/usr/bin/env python3
# Name: Muhammad Fatik Bin Imran
# Student ID: 23i-0655
# Section: C
# Assignment: 2 (Triangle Counting)

import argparse
import csv
import os
from collections import defaultdict
from statistics import mean


def read_rows(csv_path):
    rows = []
    with open(csv_path, "r", newline="") as f:
        reader = csv.DictReader(f)
        for row in reader:
            try:
                row["vertices"] = int(float(row["vertices"]))
                row["edges"] = int(float(row["edges"]))
                row["triangles"] = int(float(row["triangles"]))
                row["time_ms"] = float(row["time_ms"])
                row["speedup_vs_scalar"] = float(row["speedup_vs_scalar"])
                row["threads"] = int(float(row["threads"]))
            except (ValueError, KeyError):
                continue
            rows.append(row)
    return rows


def normalize_dataset_name(name):
    if name.endswith(".txt.gz"):
        return name[:-7]
    if name.endswith(".gz"):
        return name[:-3]
    if name.endswith(".txt"):
        return name[:-4]
    return name


def aggregate(rows):
    grouped = defaultdict(list)
    for r in rows:
        key = (normalize_dataset_name(r["dataset"]), r["variant"])
        grouped[key].append(r)

    agg = []
    for (dataset, variant), items in grouped.items():
        agg.append(
            {
                "dataset": dataset,
                "variant": variant,
                "vertices": items[0]["vertices"],
                "edges": items[0]["edges"],
                "triangles": items[0]["triangles"],
                "threads": items[0]["threads"],
                "time_ms": mean([x["time_ms"] for x in items]),
                "speedup_vs_scalar": mean([x["speedup_vs_scalar"] for x in items]),
                "runs": len(items),
            }
        )
    return sorted(agg, key=lambda x: (x["dataset"], x["time_ms"]))


def write_aggregated_csv(rows, out_csv):
    fields = [
        "dataset",
        "variant",
        "vertices",
        "edges",
        "triangles",
        "threads",
        "time_ms",
        "speedup_vs_scalar",
        "runs",
    ]
    with open(out_csv, "w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=fields)
        w.writeheader()
        for r in rows:
            w.writerow(r)


def best_per_dataset(rows):
    by_dataset = defaultdict(list)
    for r in rows:
        by_dataset[r["dataset"]].append(r)

    best = {}
    for ds, items in by_dataset.items():
        best[ds] = min(items, key=lambda x: x["time_ms"])
    return best


def write_summary_md(rows, summary_path):
    by_dataset = defaultdict(list)
    for r in rows:
        by_dataset[r["dataset"]].append(r)

    best = best_per_dataset(rows)

    lines = []
    lines.append("# Benchmark Summary")
    lines.append("")
    lines.append("## Best Variant Per Dataset")
    lines.append("")

    for ds in sorted(best.keys()):
        b = best[ds]
        lines.append(f"- **{ds}**: `{b['variant']}` at {b['time_ms']:.4f} ms (speedup {b['speedup_vs_scalar']:.3f}x, threads={b['threads']})")

    lines.append("")
    lines.append("## Detailed Results")
    lines.append("")

    for ds in sorted(by_dataset.keys()):
        lines.append(f"### {ds}")
        lines.append("")
        lines.append("| Variant | Time (ms) | Speedup vs Scalar | Triangles | Vertices | Edges | Threads |")
        lines.append("|---|---:|---:|---:|---:|---:|---:|")
        for r in sorted(by_dataset[ds], key=lambda x: x["time_ms"]):
            lines.append(
                f"| {r['variant']} | {r['time_ms']:.4f} | {r['speedup_vs_scalar']:.3f} | {r['triangles']} | {r['vertices']} | {r['edges']} | {r['threads']} |"
            )
        lines.append("")

    with open(summary_path, "w") as f:
        f.write("\n".join(lines) + "\n")


def try_make_plots(rows, out_dir):
    try:
        import matplotlib.pyplot as plt
    except Exception:
        return False, "matplotlib not available; skipped PNG plots"

    by_dataset = defaultdict(list)
    for r in rows:
        by_dataset[r["dataset"]].append(r)

    for ds in sorted(by_dataset.keys()):
        items = sorted(by_dataset[ds], key=lambda x: x["time_ms"])
        variants = [x["variant"] for x in items]
        times = [x["time_ms"] for x in items]
        speedups = [x["speedup_vs_scalar"] for x in items]

        plt.figure(figsize=(10, 5))
        plt.bar(variants, times)
        plt.title(f"Execution Time by Variant ({ds})")
        plt.ylabel("Time (ms)")
        plt.xticks(rotation=20, ha="right")
        plt.tight_layout()
        plt.savefig(os.path.join(out_dir, f"{ds}_time.png"), dpi=160)
        plt.close()

        plt.figure(figsize=(10, 5))
        plt.bar(variants, speedups)
        plt.title(f"Speedup vs Scalar by Variant ({ds})")
        plt.ylabel("Speedup")
        plt.xticks(rotation=20, ha="right")
        plt.tight_layout()
        plt.savefig(os.path.join(out_dir, f"{ds}_speedup.png"), dpi=160)
        plt.close()

    return True, "PNG plots generated"


def main():
    parser = argparse.ArgumentParser(description="Analyze triangle benchmark CSV and generate summary artifacts")
    parser.add_argument("csv", help="Input benchmark CSV")
    parser.add_argument("--out-dir", default="results/analysis", help="Output directory")
    args = parser.parse_args()

    os.makedirs(args.out_dir, exist_ok=True)

    rows = read_rows(args.csv)
    if not rows:
        raise SystemExit("No valid rows found in input CSV")

    aggregated = aggregate(rows)

    out_csv = os.path.join(args.out_dir, "aggregated_results.csv")
    summary_md = os.path.join(args.out_dir, "summary.md")

    write_aggregated_csv(aggregated, out_csv)
    write_summary_md(aggregated, summary_md)
    plots_ok, plots_msg = try_make_plots(aggregated, args.out_dir)

    print(f"Wrote: {out_csv}")
    print(f"Wrote: {summary_md}")
    print(f"Plots: {plots_msg}")

    if not plots_ok:
        print("Tip: install matplotlib to enable PNG plot generation")


if __name__ == "__main__":
    main()
