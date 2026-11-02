import csv
import statistics
from collections import defaultdict

INPUT = "results/benchmarks/part_b_timings.csv"
OUTPUT = "results/benchmarks/part_b_summary.csv"

groups = defaultdict(list)

with open(INPUT, newline="") as f:
    reader = csv.DictReader(f)

    for row in reader:
        key = (
            row["mode"],
            row["partition"],
            int(row["threads"])
        )
        groups[key].append(float(row["elapsed_seconds"]))

sequential_times = groups[("sequential", "row", 1)]
baseline = statistics.mean(sequential_times)

print(f"\nSequential baseline: {baseline:.3f} seconds\n")

results = []

for (mode, partition, threads), times in groups.items():

    if mode == "sequential":
        continue

    avg = statistics.mean(times)
    std = statistics.stdev(times) if len(times) > 1 else 0.0

    speedup = baseline / avg
    efficiency = speedup / threads

    results.append({
        "mode": mode,
        "partition": partition,
        "threads": threads,
        "average_seconds": round(avg, 4),
        "std_deviation": round(std, 4),
        "speedup": round(speedup, 4),
        "efficiency": round(efficiency, 4)
    })

results.sort(
    key=lambda x: (x["mode"], x["partition"], x["threads"])
)

with open(OUTPUT, "w", newline="") as f:
    fields = [
        "mode",
        "partition",
        "threads",
        "average_seconds",
        "std_deviation",
        "speedup",
        "efficiency"
    ]

    writer = csv.DictWriter(f, fieldnames=fields)
    writer.writeheader()
    writer.writerows(results)

print(
    f"{'Mode':<16} {'Partition':<12} {'T':>3} "
    f"{'Avg(s)':>9} {'Speedup':>9} {'Efficiency':>11}"
)
print("-" * 68)

for r in results:
    print(
        f"{r['mode']:<16} "
        f"{r['partition']:<12} "
        f"{r['threads']:>3} "
        f"{r['average_seconds']:>9.3f} "
        f"{r['speedup']:>9.3f} "
        f"{r['efficiency']:>10.1%}"
    )

print(f"\nSummary saved to: {OUTPUT}")
