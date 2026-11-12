# Parallel and Asynchronous Simulation of Pandemic Spread

## CSE600 – Assignment 3

A C++17-based project that implements and analyses the spread of an infectious disease across a two-dimensional population grid. The project explores sequential simulation, parallel processing, asynchronous programming, performance optimization, and epidemiological analysis.

The primary objective is to develop a reproducible simulation system, improve its computational performance using parallel and asynchronous techniques, and study how public-health interventions influence epidemic behaviour.

---

## 1. Project Overview

The project is divided into five parts, with each part extending the functionality and analysis developed in the previous stages.

- **Part A – Sequential Simulation:** Development of the fundamental pandemic simulation model.
- **Part B – Parallel Simulation:** Parallel execution using multithreading, thread pools, and workload partitioning strategies.
- **Part C – Asynchronous Programming:** Asynchronous snapshot writing and concurrent parameter sweeps.
- **Part D – Performance Study:** Evaluation of strong scaling, weak scaling, workload distribution, and asynchronous I/O.
- **Part E – Epidemiological Analysis:** Experiments involving infection rates, vaccination, masks, lockdown, and comparison with the SIR model.

Together, these parts demonstrate the development of a computational simulation from its basic sequential implementation to parallel execution and experimental analysis.

## 2. Technologies and Tools

The project uses the following technologies:

- **C++17:** Core implementation of the pandemic simulation.
- **CMake:** Project configuration and compilation.
- **GCC/G++:** C++ compiler.
- **POSIX threads and C++ threading facilities:** Parallel execution.
- **Python 3:** Experimental data analysis and visualization.
- **Pandas:** Processing and analysing experimental results.
- **Matplotlib:** Generating plots and visualizations.
- **Git and GitHub:** Version control and project management.
- **WSL Ubuntu:** Development and execution environment.

## 3. Pandemic Simulation Model

### 3.1 Population Representation

The population is represented using an N × N two-dimensional grid, where each cell represents one individual.

Every individual can belong to one of five possible states:

- **Susceptible (S):** An individual who can become infected.
- **Infected (I):** An individual currently carrying the infection.
- **Recovered (R):** An individual who has recovered from the infection.
- **Vaccinated (V):** An individual who has been vaccinated.
- **Dead (D):** An individual who has died due to the infection.

The simulation maintains the state of every individual over successive time steps, with each time step representing one simulation day.

### 3.2 Neighbourhood and Infection Mechanism

Each individual interacts with the eight surrounding cells, following a Moore neighbourhood.

The grid uses non-wrapping boundaries. Therefore, individuals located at the edges do not interact with cells on the opposite side of the grid.

For a susceptible individual with k infected neighbours, the infection probability is calculated as:

\[
P(k)=1-(1-\beta_{\mathrm{eff}})^k
\]

Where:

- \(k\) represents the number of infected neighbours.
- \(\beta_{\mathrm{eff}}\) represents the effective infection rate.

The effective infection rate can be modified using mask and lockdown factors:

\[
\beta_{\mathrm{eff}}=\beta \times \text{mask factor} \times \text{lockdown factor}
\]

### 3.3 Disease Progression

Infected individuals remain infected for a configurable number of simulation days.

After reaching the configured infection duration, an infected individual either recovers or dies according to the configured death probability.

The simulation tracks the population in each state throughout its execution and records daily statistics.

### 3.4 Reproducibility

The simulation uses a configurable random seed to support reproducible experiments.

Deterministic random-number generation based on the seed, simulation day, cell, and random stream helps maintain consistent simulation outcomes across supported execution configurations.

## 4. Project Structure

The repository contains the CMake configuration, implementation files, documentation, and experiment-related files.

The main project components include:

- `CMakeLists.txt` – CMake build configuration.
- `src/` – C++ implementation files.
- `include/` – Header files, where applicable in the corresponding project branches.
- `tests/` – Verification and testing code maintained with the relevant implementation.
- `scripts/` – Experiment and analysis scripts.
- `results/` – Generated simulation results, CSV files, and plots.
- `build/` – Local CMake build directory.
- `README.md` – Project documentation.
- `FINAL_REPORT.md` – Consolidated report covering Parts A–E.

Some implementation files, tests, and experiment artifacts are maintained in their respective assignment branches rather than all being present on `main`.

## 5. Build Instructions

### 5.1 Requirements

The project requires:

- A C++17-compatible compiler.
- CMake 3.16 or later.
- GCC/G++ or another compatible compiler.
- Git.

On Ubuntu or WSL Ubuntu, the required build tools can be installed using:

```bash
sudo apt update
sudo apt install build-essential cmake git
```

### 5.2 Configure the Project

From the existing project directory, configure a Release build:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
```

### 5.3 Compile the Project

Build the project using:

```bash
cmake --build build -j2
```

After a successful build, the simulation executable is generated at:

```text
build/pandemic
```

## 6. Running the Simulation

### 6.1 Display Available Options

To display the available command-line options:

```bash
./build/pandemic --help
```

### 6.2 Basic Simulation

An example execution is:

```bash
./build/pandemic --N 100 --days 50 --seed 42
```

### 6.3 Configuring Simulation Parameters

The simulation supports configurable parameters, including:

- `--N`: Grid dimension.
- `--days`: Number of simulation days.
- `--seed`: Random seed.
- `--I0`: Initial number of infected individuals.
- `--duration`: Infection duration.
- `--beta`: Base infection rate.
- `--mu`: Death probability.
- `--v`: Vaccination fraction.
- `--mask`: Mask effectiveness factor.
- `--lambda`: Lockdown factor.
- `--theta-on`: Lockdown activation threshold.
- `--theta-off`: Lockdown deactivation threshold.
- `--threads`: Number of threads.
- `--chunks`: Number of work chunks.
- `--snapshot-every`: Snapshot frequency.
- `--max-pending`: Maximum pending asynchronous tasks.
- `--io-latency`: Configurable artificial I/O latency.
- `--output`: Output CSV file path.

For the exact supported options and default values, use the executable's help command.

## 7. Implementation Details by Part

### Part A – Sequential Simulation

Part A establishes the fundamental pandemic simulation.

The implementation includes:

- Representation of individuals and their infection states.
- Initialization of the population.
- Configurable infection and disease progression parameters.
- Neighbour-based infection transmission.
- Recovery and death transitions.
- Daily population-state counting.
- CSV output containing daily simulation statistics.
- Deterministic random-number generation.

The sequential implementation provides the reference behaviour for subsequent parallel implementations.

### Part B – Parallel Simulation

Part B introduces parallel execution to improve computational performance.

The implementation explores:

- Thread-per-day execution.
- A reusable thread pool.
- Row-based partitioning.
- Block-based partitioning.
- Interleaved-row partitioning.

The parallel implementation separates reading from the current simulation state and writing to the next state. This avoids conflicting updates between workers.

Deterministic random-number generation is used to support reproducibility across different thread counts and partitioning strategies.

Correctness verification and performance benchmarks are included in the Part B work.

### Part C – Asynchronous Programming

Part C focuses on asynchronous operations and reducing waiting time caused by I/O and independent simulation tasks.

The major components include:

- Blocking snapshot writing.
- Asynchronous snapshot writing.
- Bounded pending snapshot tasks.
- Configurable artificial I/O latency.
- Asynchronous parameter sweeps.
- Concurrent execution of independent simulation configurations.
- Collection and recording of completed simulation results.

The parameter sweep evaluates combinations of infection rates, vaccination levels, and random seeds.

### Part D – Performance Study

Part D evaluates the performance characteristics of parallel and asynchronous simulation.

The experiments cover:

- **D1 – Strong Scaling:** Evaluating performance as the number of processing resources increases for a fixed problem size.
- **D2 – Weak Scaling:** Studying performance while increasing the problem size with available processing resources.
- **D3 – Work-Unit Size and Partitioning:** Comparing workload distribution strategies and their effect on performance.
- **D4 – Parallelism Within a Simulation vs. Across Independent Simulations:** Comparing different levels of parallel execution.
- **D5 – Hiding I/O Time:** Studying how asynchronous snapshot writing can reduce the impact of I/O operations on computation.

The performance study considers execution time, speedup, workload distribution, and the influence of I/O operations.

### Part E – Epidemiological Analysis

Part E uses the simulation to study epidemic behaviour under different parameter configurations.

The experiments include:

- Baseline epidemic behaviour.
- Variation in the infection rate.
- Effect of vaccination coverage.
- Effect of masks and lockdown interventions.
- Comparison with the standard SIR epidemiological model.

Multiple random seeds are used in the experiments to examine variation across simulation runs.

The analysis considers metrics such as:

- Peak infected population.
- Day on which the infection peak occurs.
- Epidemic duration.
- Attack rate.
- Outbreak probability.
- Death count.
- Effectiveness of intervention strategies.

The SIR comparison provides a reference for understanding the differences between a spatial stochastic simulation and a well-mixed epidemiological model.

## 8. Python Analysis Environment

Python is used to process experimental results and generate visualizations.

### 8.1 Create a Virtual Environment

Create a Python virtual environment:

```bash
python3 -m venv .venv
```

Activate it:

```bash
source .venv/bin/activate
```

### 8.2 Install Dependencies

Install the required Python libraries:

```bash
pip install pandas matplotlib
```

### 8.3 Run Analysis Scripts

Run the relevant analysis scripts from the repository root, using the script names available in the corresponding experiment branch.

For example, Part E analysis scripts may include:

```bash
python scripts/part_e/analyze_beta.py
python scripts/part_e/analyze_vaccination.py
python scripts/part_e/analyze_interventions.py
```

Generated visualizations can be used to compare experimental configurations and interpret epidemic trends.

## 9. Results and Experimental Outputs

Simulation and experiment outputs include:

- CSV files containing daily simulation statistics.
- Benchmark measurements for performance experiments.
- Parameter-sweep results.
- Plots and visualizations generated during epidemiological analysis.
- Experimental data used in the final report.

Generated data and build artifacts are maintained separately from source-code commits where appropriate. Required experiment outputs should be submitted through the designated course submission platform.

## 10. Final Project Report

The consolidated report documents the complete assignment, covering Parts A, B, C, D, and E.

It includes the project methodology, implementation details, experimental setup, performance analysis, epidemiological findings, limitations, and conclusions.

**Read the complete report here:**

[FINAL_REPORT.md](FINAL_REPORT.md)

## 11. Version Control and Branch Organization

Git is used to maintain the implementation and development history of the assignment.

The project work is organized across separate branches for the different assignment parts.

To inspect the available branches:

```bash
git branch -a
```

To inspect recent commits:

```bash
git log --oneline --decorate -10
```

To check the current branch:

```bash
git branch --show-current
```

The consolidated project report and main project documentation are maintained on the `main` branch.

## 12. Conclusion

This project combines software engineering, parallel and asynchronous programming, performance analysis, and epidemiological modelling.

It demonstrates how a simulation can be developed incrementally, verified for correctness, optimized through parallel execution, and used to investigate the impact of public-health interventions.

The five assignment parts collectively provide experience in building reproducible computational models and evaluating both their computational performance and their practical analytical applications.
