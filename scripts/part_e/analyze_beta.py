
import glob
import os
import re

import pandas as pd
import matplotlib.pyplot as plt

INPUT_DIR = "results/part-e/beta"
OUTPUT_DIR = "results/part-e"

POPULATION = 500 * 500
OUTBREAK_THRESHOLD = 0.01  # 1% of total population

records = []

files = sorted(glob.glob(os.path.join(INPUT_DIR, "*.csv")))

for path in files:
    filename = os.path.basename(path)

    match = re.search(r"beta_([0-9.]+)_seed_(\d+)\.csv", filename)
    if not match:
        print("Skipping unexpected filename:", filename)
        continue

    beta = float(match.group(1))
    seed = int(match.group(2))

    df = pd.read_csv(path)

    peak_index = df["I"].idxmax()
    peak_infected = int(df.loc[peak_index, "I"])
    peak_day = int(df.loc[peak_index, "day"])

    final = df.iloc[-1]

    duration = int(final["day"])
    final_recovered = int(final["R"])
    final_deaths = int(final["D"])

    final_infected = int(final["I"])

    total_ever_infected = (final_recovered + final_deaths + final_infected)
    attack_rate = total_ever_infected / POPULATION

    records.append({
        "beta": beta,
        "seed": seed,
        "peak_infected": peak_infected,
        "peak_day": peak_day,
        "duration_days": duration,
        "epidemic_ended": final_infected == 0,
        "total_ever_infected": total_ever_infected,
        "attack_rate": attack_rate,
        "outbreak": attack_rate >= OUTBREAK_THRESHOLD
    })

runs = pd.DataFrame(records)

if runs.empty:
    raise RuntimeError("No valid beta result files found.")

runs.to_csv(
    os.path.join(OUTPUT_DIR, "beta_individual_results.csv"),
    index=False
)

summary = (
    runs.groupby("beta")
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
    os.path.join(OUTPUT_DIR, "beta_summary.csv"),
    index=False
)

print("\nBETA EXPERIMENT SUMMARY")
print("=======================")
print(summary.round(4).to_string(index=False))

# Plot 1: Average peak infected
plt.figure(figsize=(9, 5))
plt.errorbar(
    summary["beta"],
    summary["avg_peak_infected"],
    yerr=summary["sd_peak_infected"].fillna(0),
    marker="o",
    capsize=5
)
plt.xlabel("Infection Rate (Beta)")
plt.ylabel("Average Peak Infected")
plt.title("Effect of Beta on Peak Infections")
plt.grid(True, alpha=0.3)
plt.tight_layout()
plt.savefig(
    os.path.join(OUTPUT_DIR, "plots/beta_peak_infected.png"),
    dpi=300
)
plt.close()

# Plot 2: Average attack rate
plt.figure(figsize=(9, 5))
plt.errorbar(
    summary["beta"],
    summary["avg_attack_rate"] * 100,
    yerr=summary["sd_attack_rate"].fillna(0) * 100,
    marker="o",
    capsize=5
)
plt.xlabel("Infection Rate (Beta)")
plt.ylabel("Average Attack Rate (%)")
plt.title("Effect of Beta on Total Population Infected")
plt.grid(True, alpha=0.3)
plt.tight_layout()
plt.savefig(
    os.path.join(OUTPUT_DIR, "plots/beta_attack_rate.png"),
    dpi=300
)
plt.close()

print("\nFiles generated:")
print("results/part-e/beta_individual_results.csv")
print("results/part-e/beta_summary.csv")
print("results/part-e/plots/beta_peak_infected.png")
print("results/part-e/plots/beta_attack_rate.png")
