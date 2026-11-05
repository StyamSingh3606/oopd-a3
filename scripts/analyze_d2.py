import pandas as pd
import matplotlib.pyplot as plt

raw = pd.read_csv("results/benchmarks/d2_weak_scaling_raw.csv")

summary = raw.groupby(["threads", "N", "total_cells"], as_index=False).agg(
    median_seconds=("elapsed_seconds", "median"),
    min_seconds=("elapsed_seconds", "min"),
    max_seconds=("elapsed_seconds", "max")
)

baseline = summary.loc[summary["threads"] == 1, "median_seconds"].iloc[0]
summary["cells_per_thread"] = summary["total_cells"] / summary["threads"]
summary["weak_scaling_efficiency"] = baseline / summary["median_seconds"]

summary.to_csv("results/benchmarks/d2_summary.csv", index=False)

print("\nD2 WEAK SCALING SUMMARY")
print(summary[[
    "threads", "N", "cells_per_thread", "median_seconds",
    "min_seconds", "max_seconds", "weak_scaling_efficiency"
]].to_string(index=False, formatters={
    "cells_per_thread": "{:,.0f}".format,
    "median_seconds": "{:.3f}".format,
    "min_seconds": "{:.3f}".format,
    "max_seconds": "{:.3f}".format,
    "weak_scaling_efficiency": "{:.2%}".format
}))

plt.figure()
plt.plot(summary["threads"], summary["median_seconds"], marker="o")
plt.xlabel("Number of threads")
plt.ylabel("Median execution time (seconds)")
plt.title("D2: Weak Scaling - Execution Time")
plt.xticks(summary["threads"])
plt.grid(True)
plt.savefig("results/benchmarks/d2_execution_time.png", dpi=200, bbox_inches="tight")
plt.close()

plt.figure()
plt.plot(summary["threads"], summary["weak_scaling_efficiency"] * 100, marker="o")
plt.xlabel("Number of threads")
plt.ylabel("Weak scaling efficiency (%)")
plt.title("D2: Weak Scaling Efficiency")
plt.xticks(summary["threads"])
plt.ylim(0, 110)
plt.grid(True)
plt.savefig("results/benchmarks/d2_efficiency.png", dpi=200, bbox_inches="tight")
plt.close()

print("\nGenerated:")
print("results/benchmarks/d2_summary.csv")
print("results/benchmarks/d2_execution_time.png")
print("results/benchmarks/d2_efficiency.png")
