import pandas as pd
import matplotlib.pyplot as plt

raw = pd.read_csv("results/benchmarks/d3_partition_raw.csv")

summary = raw.groupby("partition", as_index=False).agg(
    median_seconds=("elapsed_seconds", "median"),
    min_seconds=("elapsed_seconds", "min"),
    max_seconds=("elapsed_seconds", "max")
)

summary = summary.sort_values("median_seconds")
best = summary.iloc[0]

summary.to_csv("results/benchmarks/d3_partition_summary.csv", index=False)

print("\nD3.2 PARTITIONING SUMMARY")
print(summary.to_string(index=False, formatters={
    "median_seconds": "{:.3f}".format,
    "min_seconds": "{:.3f}".format,
    "max_seconds": "{:.3f}".format
}))

print(f"\nFastest partition: {best['partition']}")
print(f"Median time: {best['median_seconds']:.3f} seconds")

plt.figure()
plt.bar(summary["partition"], summary["median_seconds"])
plt.xlabel("Partitioning strategy")
plt.ylabel("Median execution time (seconds)")
plt.title("D3.2: Partitioning Strategy Comparison")
plt.grid(axis="y")
plt.savefig("results/benchmarks/d3_partition_comparison.png",
            dpi=200, bbox_inches="tight")
plt.close()

print("\nGenerated:")
print("results/benchmarks/d3_partition_summary.csv")
print("results/benchmarks/d3_partition_comparison.png")
