
import argparse
import os

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt


def simulate_sir(population, initial_infected, beta, gamma, max_days):
    susceptible = population - initial_infected
    infected = initial_infected
    recovered = 0.0

    records = [{
        "day": 0,
        "S": susceptible,
        "I": infected,
        "R": recovered
    }]

    for day in range(1, max_days + 1):
        # New infections and recoveries during this day
        new_infections = beta * susceptible * infected / population
        new_recoveries = gamma * infected

        susceptible -= new_infections
        infected += new_infections - new_recoveries
        recovered += new_recoveries

        records.append({
            "day": day,
            "S": susceptible,
            "I": infected,
            "R": recovered
        })

        if infected < 1e-6:
            break

    return pd.DataFrame(records)


def main():
    parser = argparse.ArgumentParser(
        description="Deterministic standard SIR model"
    )

    parser.add_argument("--N", type=int, default=250000)
    parser.add_argument("--I0", type=int, default=10)
    parser.add_argument("--beta", type=float, default=0.08)
    parser.add_argument("--duration", type=int, default=7)
    parser.add_argument("--days", type=int, default=2000)

    args = parser.parse_args()

    gamma = 1.0 / args.duration

    output_dir = "results/part-e"
    plot_dir = os.path.join(output_dir, "plots")

    os.makedirs(plot_dir, exist_ok=True)

    df = simulate_sir(
        population=args.N,
        initial_infected=args.I0,
        beta=args.beta,
        gamma=gamma,
        max_days=args.days
    )

    csv_path = os.path.join(output_dir, "standard_sir.csv")
    df.to_csv(csv_path, index=False)

    peak_index = df["I"].idxmax()
    peak_infected = df.loc[peak_index, "I"]
    peak_day = int(df.loc[peak_index, "day"])

    final = df.iloc[-1]

    plt.figure(figsize=(11, 6))
    plt.plot(df["day"], df["S"], label="Susceptible")
    plt.plot(df["day"], df["I"], label="Infected")
    plt.plot(df["day"], df["R"], label="Recovered")

    plt.xlabel("Day")
    plt.ylabel("Population")
    plt.title("Standard Deterministic SIR Model")
    plt.legend()
    plt.grid(alpha=0.3)
    plt.tight_layout()

    plot_path = os.path.join(plot_dir, "standard_sir.png")
    plt.savefig(plot_path, dpi=150)
    plt.close()

    print("\nSTANDARD SIR RESULTS")
    print("--------------------")
    print(f"Population: {args.N}")
    print(f"Initial infected: {args.I0}")
    print(f"Beta: {args.beta}")
    print(f"Gamma: {gamma:.6f}")
    print(f"Infection duration: {args.duration} days")
    print(f"Peak infected: {peak_infected:.2f}")
    print(f"Peak day: {peak_day}")
    print(f"Simulation duration: {int(final['day'])} days")
    print(f"Final susceptible: {final['S']:.2f}")
    print(f"Final infected: {final['I']:.6f}")
    print(f"Final recovered: {final['R']:.2f}")
    print(f"\nCSV saved: {csv_path}")
    print(f"Plot saved: {plot_path}")


if __name__ == "__main__":
    main()
