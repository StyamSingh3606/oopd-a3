import pandas as pd
import matplotlib.pyplot as plt

raw = pd.read_csv("results/benchmarks/d3_modes_raw.csv")

summary = raw.groupby(["N", "mode"], as_index=False).agg(
    median_seconds=("elapsed_seconds", "median"),
    min_seconds=("elapsed_seconds", "min"),
    max_seconds=("elapsed_seconds", "max")
)

summary.to_csv("results/benchmarks/d3_modes_summary.csv", index=False)

print("\nD3.3 MODE COMPARISON")
print(summary.to_string(index=False, formatters={
    "median_seconds": "{:.4f}".format,
    "min_seconds": "{:.4f}".format,
    "max_seconds": "{:.4f}".format
}))

pivot = summary.pivot(index="N", columns="mode", values="median_seconds")

print("\nSPEEDUP COMPARISON (thread-per-day / thread-pool)")
print((pivot["thread-per-day"] / pivot["thread-pool"]).to_string())

plt.figure()
for mode in pivot.columns:
    plt.plot(pivot.index, pivot[mode], marker="o", label=mode)

plt.xlabel("Grid size N (N x N)")
plt.ylabel("Median execution time (seconds)")
plt.title("D3.3: Thread-per-day vs Thread-pool")
plt.legend()
plt.grid(True)
plt.savefig("results/benchmarks/d3_modes_comparison.png",
            dpi=200, bbox_inches="tight")
plt.close()

print("\nGenerated:")
print("results/benchmarks/d3_modes_summary.csv")
print("results/benchmarks/d3_modes_comparison.png")
