import pandas as pd
import matplotlib.pyplot as plt

raw = pd.read_csv("results/benchmarks/d3_chunks_raw.csv")

summary = raw.groupby("chunks", as_index=False).agg(
    median_seconds=("elapsed_seconds", "median"),
    min_seconds=("elapsed_seconds", "min"),
    max_seconds=("elapsed_seconds", "max")
)

best = summary.loc[summary["median_seconds"].idxmin()]

summary.to_csv("results/benchmarks/d3_chunks_summary.csv", index=False)

print("\nD3.1 CHUNK SIZE SUMMARY")
print(summary.to_string(index=False, formatters={
    "median_seconds": "{:.3f}".format,
    "min_seconds": "{:.3f}".format,
    "max_seconds": "{:.3f}".format
}))

print(f"\nBest chunk size: {int(best['chunks'])}")
print(f"Median execution time: {best['median_seconds']:.3f} seconds")

plt.figure()
plt.plot(summary["chunks"], summary["median_seconds"], marker="o")
plt.xlabel("Number of chunks")
plt.ylabel("Median execution time (seconds)")
plt.title("D3.1: Effect of Chunk Size")
plt.xticks(summary["chunks"])
plt.grid(True)
plt.savefig("results/benchmarks/d3_chunks_execution_time.png",
            dpi=200, bbox_inches="tight")
plt.close()

print("\nGenerated:")
print("results/benchmarks/d3_chunks_summary.csv")
print("results/benchmarks/d3_chunks_execution_time.png")
