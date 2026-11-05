
import csv
import statistics
import os
import matplotlib.pyplot as plt

INPUT = "results/benchmarks/d1_strong_scaling_raw.csv"
OUTPUT_DIR = "results/benchmarks"

os.makedirs(OUTPUT_DIR, exist_ok=True)

# Read raw benchmark data
data = {}

with open(INPUT, newline="") as f:
    reader = csv.DictReader(f)

    for row in reader:
        threads = int(row["threads"])
        elapsed = float(row["elapsed_seconds"])
        checksum = row["checksum"]
        days = int(row["executed_days"])

        data.setdefault(threads, []).append(elapsed)

        if checksum != "11331261963425922907" or days != 200:
            raise ValueError(
                f"Correctness check failed for {threads} threads"
            )

# Calculate median, minimum and maximum
threads_list = sorted(data.keys())
baseline = statistics.median(data[1])

summary = []

for t in threads_list:
    times = data[t]

    median_time = statistics.median(times)
    minimum = min(times)
    maximum = max(times)

    speedup = baseline / median_time
    efficiency = speedup / t

    if t == 1:
        karp_flatt = 0.0
    else:
        karp_flatt = (
            (1 / speedup - 1 / t) / (1 - 1 / t)
        )

    summary.append({
        "threads": t,
        "median_seconds": median_time,
        "min_seconds": minimum,
        "max_seconds": maximum,
        "speedup": speedup,
        "efficiency": efficiency,
        "karp_flatt": karp_flatt
    })

# Save summary CSV
summary_file = f"{OUTPUT_DIR}/d1_summary.csv"

with open(summary_file, "w", newline="") as f:
    fields = summary[0].keys()
    writer = csv.DictWriter(f, fieldnames=fields)
    writer.writeheader()
    writer.writerows(summary)

# Print summary
print("\nD1 STRONG SCALING SUMMARY")
print("-" * 95)
print(
    f"{'Threads':>8} {'Median(s)':>12} "
    f"{'Min(s)':>12} {'Max(s)':>12} "
    f"{'Speedup':>10} {'Efficiency':>12} {'Karp-Flatt':>12}"
)

for r in summary:
    print(
        f"{r['threads']:>8} "
        f"{r['median_seconds']:>12.3f} "
        f"{r['min_seconds']:>12.3f} "
        f"{r['max_seconds']:>12.3f} "
        f"{r['speedup']:>10.3f} "
        f"{r['efficiency'] * 100:>11.2f}% "
        f"{r['karp_flatt']:>12.4f}"
    )

# Graph 1: Execution time
plt.figure()
plt.errorbar(
    threads_list,
    [r["median_seconds"] for r in summary],
    yerr=[
        [
            r["median_seconds"] - r["min_seconds"]
            for r in summary
        ],
        [
            r["max_seconds"] - r["median_seconds"]
            for r in summary
        ]
    ],
    marker="o",
    capsize=5
)
plt.xlabel("Number of Threads")
plt.ylabel("Execution Time (seconds)")
plt.title("D1: Strong Scaling - Execution Time")
plt.xticks(threads_list)
plt.grid(True)
plt.savefig(f"{OUTPUT_DIR}/d1_execution_time.png", dpi=200)
plt.close()

# Graph 2: Speedup
plt.figure()
plt.plot(
    threads_list,
    [r["speedup"] for r in summary],
    marker="o",
    label="Measured speedup"
)
plt.plot(
    threads_list,
    threads_list,
    linestyle="--",
    label="Ideal speedup"
)
plt.xlabel("Number of Threads")
plt.ylabel("Speedup")
plt.title("D1: Strong Scaling - Speedup")
plt.xticks(threads_list)
plt.grid(True)
plt.legend()
plt.savefig(f"{OUTPUT_DIR}/d1_speedup.png", dpi=200)
plt.close()

# Graph 3: Parallel efficiency
plt.figure()
plt.plot(
    threads_list,
    [r["efficiency"] * 100 for r in summary],
    marker="o"
)
plt.xlabel("Number of Threads")
plt.ylabel("Parallel Efficiency (%)")
plt.title("D1: Strong Scaling - Parallel Efficiency")
plt.xticks(threads_list)
plt.grid(True)
plt.savefig(f"{OUTPUT_DIR}/d1_efficiency.png", dpi=200)
plt.close()

print("\nGenerated:")
print(summary_file)
print(f"{OUTPUT_DIR}/d1_execution_time.png")
print(f"{OUTPUT_DIR}/d1_speedup.png")
print(f"{OUTPUT_DIR}/d1_efficiency.png")
