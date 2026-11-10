
import os
import pandas as pd
import matplotlib.pyplot as plt

spatial_path = "results/part-e/baseline_no_intervention_1000.csv"
sir_path = "results/part-e/standard_sir.csv"

spatial = pd.read_csv(spatial_path)
sir = pd.read_csv(sir_path)

# Spatial model: recovered + dead = removed
spatial["Removed"] = spatial["R"] + spatial["D"]

# Standard SIR: recovered = removed
sir["Removed"] = sir["R"]

os.makedirs("results/part-e/plots", exist_ok=True)

# Plot infected curves
plt.figure(figsize=(11, 6))
plt.plot(spatial["day"], spatial["I"], label="Spatial Model")
plt.plot(sir["day"], sir["I"], label="Standard SIR")

plt.xlabel("Day")
plt.ylabel("Infected Population")
plt.title("Infection Curve: Spatial Model vs Standard SIR")
plt.legend()
plt.grid(alpha=0.3)
plt.tight_layout()
plt.savefig(
    "results/part-e/plots/sir_vs_spatial_infected.png",
    dpi=150
)
plt.close()

# Plot all compartments
fig, axes = plt.subplots(1, 2, figsize=(15, 6))

axes[0].plot(spatial["day"], spatial["S"], label="S")
axes[0].plot(spatial["day"], spatial["I"], label="I")
axes[0].plot(spatial["day"], spatial["Removed"], label="Removed (R+D)")
axes[0].set_title("Spatial Model")
axes[0].set_xlabel("Day")
axes[0].set_ylabel("Population")
axes[0].legend()
axes[0].grid(alpha=0.3)

axes[1].plot(sir["day"], sir["S"], label="S")
axes[1].plot(sir["day"], sir["I"], label="I")
axes[1].plot(sir["day"], sir["Removed"], label="R")
axes[1].set_title("Standard SIR Model")
axes[1].set_xlabel("Day")
axes[1].set_ylabel("Population")
axes[1].legend()
axes[1].grid(alpha=0.3)

plt.tight_layout()
plt.savefig(
    "results/part-e/plots/sir_vs_spatial_compartments.png",
    dpi=150
)
plt.close()

print("Comparison graphs generated successfully.")
print("Spatial peak:", int(spatial["I"].max()))
print("Spatial peak day:", int(spatial.loc[spatial["I"].idxmax(), "day"]))
print("SIR peak:", round(sir["I"].max(), 2))
print("SIR peak day:", int(sir.loc[sir["I"].idxmax(), "day"]))
print("Graphs saved in results/part-e/plots/")
