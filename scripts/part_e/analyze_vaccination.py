
import glob
import os
import re

import pandas as pd
import matplotlib.pyplot as plt

INPUT_DIR = "results/part-e/vaccination"
OUTPUT_DIR = "results/part-e"

POPULATION = 500 * 500
OUTBREAK_THRESHOLD = 0.01  # 1% of total population

records = []

files = sorted(glob.glob(os.path.join(INPUT_DIR, "*.csv")))

for path in files:
    filename = os.path.basename(path)

    match = re.search(r"v_([0-9.]+)_seed_(\d+)\.csv", filename)
    if not match:
        print("Skipping unexpected filename:", filename)
        continue

    vaccination = float(match.group(1))
    seed = int(match.group(2))

    df = pd.read_csv(path)

    peak_index = df["I"].idxmax()
    peak_infected = int(df.loc[peak_index, "I"])
    peak_day = int(df.loc[peak_index, "day"])

    final = df.iloc[-1]

    final_infected = int(final["I"])
    final_recovered = int(final["R"])
    final_deaths = int(final["D"])

    total_ever_infected = (
        final_recovered + final_deaths + final_infected
    )

    attack_rate = total_ever_infected / POPULATION

    records.append({
        "vaccination": vaccination,
        "seed": seed,
        "peak_infected": peak_infected,
        "peak_day": peak_day,
        "duration_days": int(final["day"]),
        "total_ever_infected": total_ever_infected,
        "attack_rate": attack_rate,
        "outbreak": attack_rate >= OUTBREAK_THRESHOLD,
        "epidemic_ended": final_infected == 0
    })

runs = pd.DataFrame(records)

if runs.empty:
    raise RuntimeError("No valid vaccination result files found.")

runs.to_csv(
    os.path.join(OUTPUT_DIR, "vaccination_individual_results.csv"),
    index=False
)

summary = (
    runs.groupby("vaccination")
    .agg(
        runs=("seed", "count"),
        avg_peak_infected=("peak_infected", "mean"),
        sd_peak_infected=("peak_infected", "std"),
        avg_peak_day=("peak_day", "mean"),
        sd_peak_day=("peak_day", "std"),
        avg_duration=("duration_days", "mean"),
        sd_duration=("duration_days", "std"),
        completed_runs=("epidemic_ended", "sum"),
        avg_attack_rate=("attack_rate", "mean"),
        sd_attack_rate=("attack_rate", "std"),
        outbreak_count=("outbreak", "sum")
    )
    .reset_index()
)

summary["outbreak_probability"] = (
    summary["outbreak_count"] / summary["runs"]
)

summary.to_csv(
    os.path.join(OUTPUT_DIR, "vaccination_summary.csv"),
    index=False
)

print("\nVACCINATION EXPERIMENT SUMMARY")
print("==============================")
print(summary.round(4).to_string(index=False))

# Plot 1: Vaccination vs average peak infections
plt.figure(figsize=(9, 5))
plt.errorbar(
    summary["vaccination"] * 100,
    summary["avg_peak_infected"],
    yerr=summary["sd_peak_infected"].fillna(0),
    marker="o",
    capsize=5
)
plt.xlabel("Vaccination Coverage (%)")
plt.ylabel("Average Peak Infected")
plt.title("Effect of Vaccination on Peak Infections")
plt.grid(True, alpha=0.3)
plt.tight_layout()
plt.savefig(
    os.path.join(OUTPUT_DIR, "plots/vaccination_peak.png"),
    dpi=300
)
plt.close()

# Plot 2: Vaccination vs attack rate
plt.figure(figsize=(9, 5))
plt.errorbar(
    summary["vaccination"] * 100,
    summary["avg_attack_rate"] * 100,
    yerr=summary["sd_attack_rate"].fillna(0) * 100,
    marker="o",
    capsize=5
)
plt.xlabel("Vaccination Coverage (%)")
plt.ylabel("Average Attack Rate (%)")
plt.title("Effect of Vaccination on Total Infections")
plt.grid(True, alpha=0.3)
plt.tight_layout()
plt.savefig(
    os.path.join(OUTPUT_DIR, "plots/vaccination_attack_rate.png"),
    dpi=300
)
plt.close()

# Plot 3: Outbreak probability
plt.figure(figsize=(9, 5))
plt.plot(
    summary["vaccination"] * 100,
    summary["outbreak_probability"] * 100,
    marker="o"
)
plt.xlabel("Vaccination Coverage (%)")
plt.ylabel("Significant Outbreak Probability (%)")
plt.title("Vaccination vs Outbreak Probability")
plt.ylim(-5, 105)
plt.grid(True, alpha=0.3)
plt.tight_layout()
plt.savefig(
    os.path.join(OUTPUT_DIR, "plots/vaccination_outbreak_probability.png"),
    dpi=300
)
plt.close()

print("\nFiles generated:")
print("results/part-e/vaccination_individual_results.csv")
print("results/part-e/vaccination_summary.csv")
print("results/part-e/plots/vaccination_peak.png")
print("results/part-e/plots/vaccination_attack_rate.png")
print("results/part-e/plots/vaccination_outbreak_probability.png")
