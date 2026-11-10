# Parallel and Asynchronous Simulation of Pandemic Spread

## 1. Project Overview

This project implements a pandemic spread simulation on a two-dimensional population grid. It explores sequential execution, multithreading, asynchronous programming, and performance optimization.

The simulation models the spread of an infectious disease and studies how infection rates, vaccination, masks, and lockdown measures affect epidemic behaviour.

The project is divided into five parts:

- **Part A:** Sequential simulation.
- **Part B:** Parallel simulation using threads and a thread pool.
- **Part C:** Asynchronous snapshot writing and parameter sweeps.
- **Part D:** Performance analysis and benchmarking.
- **Part E:** Epidemiological experiments and comparison with the standard SIR model.

## 2. Technologies Used

- C++17
- CMake
- GCC / G++
- POSIX threads and C++ threading facilities
- Python 3
- Pandas
- Matplotlib
- Git and GitHub
- WSL Ubuntu

## 3. Simulation Model

The population is represented by an N × N grid, with one individual per cell.

Each individual can be in one of five states:

| State | Description |
|---|---|
| S | Susceptible |
| I | Infected |
| R | Recovered |
| V | Vaccinated |
| D | Dead |

Each individual interacts with the eight surrounding cells (Moore neighbourhood). The grid does not wrap around at its boundaries.

The infection probability for a susceptible individual with k infected neighbours is:

\[
P(k)=1-(1-\beta_{\mathrm{eff}})^k
\]

The effective infection rate can be modified by masks and lockdown measures.

## 4. Project Structure

```text
oopd-a3/
│
├── include/                 # Header files
├── src/                     # C++ source files
├── scripts/                 # Experiment and analysis scripts
│   └── part_e/              # Part E experiment scripts
│
├── results/                 # CSV results and generated plots
│   └── part-e/
│       └── plots/
│
├── tests/                   # Test files
├── build/                   # CMake build directory
│
├── CMakeLists.txt           # CMake configuration
├── README.md                # Project documentation
└── REPORT_PART_E.md         # Part E experimental report
```

## 5. Build Instructions

### Requirements

Install the required build tools on Ubuntu:

```bash
sudo apt update
sudo apt install build-essential cmake git python3 python3-venv
```

### Clone the repository

```bash
git clone https://github.com/StyamSingh3606/oopd-a3.git
cd oopd-a3
```

### Configure the project

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
```

### Compile

```bash
cmake --build build -j2
```

The simulation executable is generated at:

```text
build/pandemic
```

## 6. Running the Simulation

A basic simulation can be executed using:

```bash
./build/pandemic --N 500 --days 1000 --seed 42
```

Example using the thread pool:

```bash
./build/pandemic \
  --N 500 \
  --days 1000 \
  --seed 42 \
  --mode thread-pool \
  --partition block \
  --threads 4 \
  --output results/output.csv
```

The command-line options allow the user to configure the grid size, simulation duration, random seed, execution mode, partitioning strategy, thread count, and output file.

Additional model parameters include infection rate, vaccination, mask factor, and lockdown settings.

Use the program's available command-line options when configuring experiments.

## 7. Parts Implemented

### Part A – Sequential Simulation

Implemented the single-threaded pandemic simulation with configurable parameters and CSV output.

### Part B – Threaded Simulation

Implemented parallel execution using:

- Thread-per-day execution.
- A reusable thread pool.
- Row-based partitioning.
- Block-based partitioning.
- Interleaved-row partitioning.

Deterministic random number generation ensures reproducible results across different thread counts and partitioning strategies.

### Part C – Asynchronous Programming

Implemented:

- Blocking snapshot writing.
- Asynchronous snapshot writing.
- Bounded pending snapshot tasks.
- Configurable artificial I/O latency.
- Asynchronous parameter sweeps.

### Part D – Performance Study

Conducted performance experiments covering:

- D1: Strong scaling.
- D2: Weak scaling.
- D3: Work-unit size and partitioning.
- D4: Parallelism within a run versus across independent runs.
- D5: Hiding I/O time using asynchronous snapshots.

The experiments include benchmark data, plots, and performance analysis.

### Part E – Epidemiological Analysis

Conducted experiments to study:

- Baseline epidemic behaviour.
- Variation in infection rate (beta).
- Effect of vaccination coverage.
- Effect of masks and lockdown.
- Comparison with the standard SIR model.

The Part E experiments use multiple random seeds and report averages and variation across runs.

See `REPORT_PART_E.md` for the detailed Part E results and analysis.

## 8. Python Analysis Environment

Create and activate a virtual environment:

```bash
python3 -m venv .venv
source .venv/bin/activate
```

Install the required packages:

```bash
pip install pandas matplotlib
```

Run the relevant analysis scripts from the project root.

For example:

```bash
python scripts/part_e/analyze_beta.py
python scripts/part_e/analyze_vaccination.py
python scripts/part_e/analyze_interventions.py
```

## 9. Results and Reports

Experimental outputs are organized under the `results/` directory.

- CSV files contain simulation and benchmark results.
- Plot files contain generated visualizations.
- Python scripts perform experiment analysis and generate graphs.
- `REPORT_PART_E.md` documents the epidemiological experiments.

## 10. Version Control

The project uses separate Git branches for assignment parts.

The completed Part B, Part C, Part D, and Part E work is maintained in their respective branches.

To inspect the current branch:

```bash
git branch
```

To inspect recent commits:

```bash
git log --oneline --decorate -5
```
