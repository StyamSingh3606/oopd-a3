import glob
import os
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

BASE = "results/part-e/interventions"
PLOTS = "results/part-e/plots"
POPULATION = 500 * 500
OUTBREAK_THRESHOLD = 0.01

os.makedirs(PLOTS, exist_ok=True)

rows = []

for path in sorted(glob.glob(os.path.join(BASE, "*.csv"))):
    filename = os.path.basename(path)

    # Ignore temporary test files; analyse only seed-based runs.
    if "_seed" not in filename:
        continue

    name = filename.rsplit("_seed", 1)[0]
    df = pd.read_csv(path)

    peak_idx = df["I"].idxmax()
    peak = df.loc[peak_idx]

    final = df.iloc[-1]

    attack_rate = (
        final["R"] + final["D"] + final["I"]
    ) / POPULATION

    rows.append({
        "configuration": name,
        "seed": int(os.path.basename(path).split("_seed")[1].split(".")[0]),
        "peak_infected": int(df["I"].max()),
        "peak_day": int(peak["day"]),
        "duration": int(final["day"]),
        "attack_rate": attack_rate,
        "epidemic_ended": final["I"] == 0,
        "outbreak": attack_rate >= OUTBREAK_THRESHOLD,
    })

data = pd.DataFrame(rows)

summary = data.groupby("configuration").agg(
    runs=("seed", "count"),
    avg_peak_infected=("peak_infected", "mean"),
    sd_peak_infected=("peak_infected", "std"),
    avg_peak_day=("peak_day", "mean"),
    sd_peak_day=("peak_day", "std"),
    avg_duration=("duration", "mean"),
    sd_duration=("duration", "std"),
    completed_runs=("epidemic_ended", "sum"),
    avg_attack_rate=("attack_rate", "mean"),
    sd_attack_rate=("attack_rate", "std"),
    outbreak_count=("outbreak", "sum"),
).reset_index()

summary["outbreak_probability"] = (
    summary["outbreak_count"] / summary["runs"]
)

baseline = summary.loc[
    summary["configuration"] == "no_intervention",
    "avg_peak_infected"
].iloc[0]

summary["peak_reduction_percent"] = (
    (baseline - summary["avg_peak_infected"]) / baseline * 100
)

summary.to_csv(
    os.path.join(BASE, "intervention_summary.csv"),
    index=False
)

print("\nINTERVENTION SUMMARY")
print(summary.round(4).to_string(index=False))

# Plot 1: Average peak infected
plt.figure(figsize=(10, 6))
plt.bar(summary["configuration"], summary["avg_peak_infected"])
plt.ylabel("Average peak infected")
plt.title("Effect of Masks and Lockdowns on Peak Infections")
plt.xticks(rotation=25, ha="right")
plt.tight_layout()
plt.savefig(os.path.join(PLOTS, "intervention_peak.png"), dpi=200)
plt.close()

# Plot 2: Average attack rate
plt.figure(figsize=(10, 6))
plt.bar(summary["configuration"], summary["avg_attack_rate"] * 100)
plt.ylabel("Average attack rate (%)")
plt.title("Effect of Interventions on Attack Rate")
plt.xticks(rotation=25, ha="right")
plt.tight_layout()
plt.savefig(os.path.join(PLOTS, "intervention_attack_rate.png"), dpi=200)
plt.close()

# Plot 3: Average peak day
plt.figure(figsize=(10, 6))
plt.bar(summary["configuration"], summary["avg_peak_day"])
plt.ylabel("Average peak day")
plt.title("Effect of Interventions on Peak Timing")
plt.xticks(rotation=25, ha="right")
plt.tight_layout()
plt.savefig(os.path.join(PLOTS, "intervention_peak_day.png"), dpi=200)
plt.close()

# Plot 4: Outbreak probability
plt.figure(figsize=(10, 6))
plt.bar(
    summary["configuration"],
    summary["outbreak_probability"] * 100
)
plt.ylabel("Outbreak probability (%)")
plt.title("Probability of Significant Outbreak")
plt.xticks(rotation=25, ha="right")
plt.tight_layout()
plt.savefig(os.path.join(PLOTS, "intervention_outbreak_probability.png"), dpi=200)
plt.close()

print("\nPlots saved in:", PLOTS)
print("Summary saved in:", os.path.join(BASE, "intervention_summary.csv"))
