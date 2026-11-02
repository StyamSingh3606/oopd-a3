import pandas as pd
import matplotlib.pyplot as plt

df = pd.read_csv("results/benchmarks/part_b_summary.csv")

# Execution time comparison
plt.figure(figsize=(10, 6))

for (mode, partition), group in df.groupby(["mode", "partition"]):
    group = group.sort_values("threads")
    plt.plot(
        group["threads"],
        group["average_seconds"],
        marker="o",
        label=f"{mode} - {partition}"
    )

plt.xlabel("Number of Threads")
plt.ylabel("Average Execution Time (seconds)")
plt.title("Part B: Execution Time vs Thread Count")
plt.xticks([1, 2, 4, 8, 16])
plt.grid(True)
plt.legend()
plt.tight_layout()
plt.savefig("results/benchmarks/execution_time.png", dpi=300)
plt.close()

# Speedup comparison
plt.figure(figsize=(10, 6))

for (mode, partition), group in df.groupby(["mode", "partition"]):
    group = group.sort_values("threads")
    plt.plot(
        group["threads"],
        group["speedup"],
        marker="o",
        label=f"{mode} - {partition}"
    )

plt.xlabel("Number of Threads")
plt.ylabel("Speedup")
plt.title("Part B: Speedup vs Thread Count")
plt.xticks([1, 2, 4, 8, 16])
plt.grid(True)
plt.legend()
plt.tight_layout()
plt.savefig("results/benchmarks/speedup.png", dpi=300)
plt.close()

print("Graphs generated successfully!")
