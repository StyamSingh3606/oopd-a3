
import pandas as pd
import matplotlib.pyplot as plt

# Input CSV
input_file = "results/part-e/baseline_no_intervention_1000.csv"

# Output graph
output_file = "results/part-e/plots/baseline_epidemic_curve.png"

# Read simulation data
df = pd.read_csv(input_file)

# Convert columns to numeric values
columns = ["day", "S", "I", "R", "D"]
for col in columns:
    df[col] = pd.to_numeric(df[col])

# Plot epidemic curves
plt.figure(figsize=(12, 7))

plt.plot(df["day"], df["S"], label="Susceptible (S)")
plt.plot(df["day"], df["I"], label="Infected (I)")
plt.plot(df["day"], df["R"], label="Recovered (R)")
plt.plot(df["day"], df["D"], label="Deaths (D)")

plt.xlabel("Simulation Day")
plt.ylabel("Number of People")
plt.title("Baseline Pandemic Simulation (No Intervention)")

plt.legend()
plt.grid(True, alpha=0.3)
plt.tight_layout()

plt.savefig(output_file, dpi=300)
plt.show()

print("Graph saved at:", output_file)
print("Total simulated days:", len(df) - 1)
print("Peak infected:", df["I"].max())
print("Peak infection day:", df.loc[df["I"].idxmax(), "day"])
