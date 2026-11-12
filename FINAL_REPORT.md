# Parallel and Asynchronous Simulation of Pandemic Spread
## Project Report 
## CSE600 – Assignment 3

## 1. Introduction

The objective of this project is to design, implement, and evaluate a pandemic-spread simulation using C++17. The simulation models how an infectious disease spreads through a population arranged on a two-dimensional grid. It also investigates how parallel programming and asynchronous execution can improve the performance of computationally intensive simulations.

The project is divided into five parts, each extending the functionality and analytical capabilities of the previous one:

- **Part A — Sequential Simulation:** Development of the basic pandemic model, including population initialization, infection transmission, recovery, death, vaccination, and lockdown mechanisms.
- **Part B — Parallel Simulation:** Implementation of multithreaded execution strategies to accelerate simulation by distributing computational work across CPU cores.
- **Part C — Asynchronous Execution:** Implementation of asynchronous snapshot writing and parameter sweeps to overlap computation with I/O and execute independent simulation experiments concurrently.
- **Part D — Performance Study:** Experimental evaluation of parallel execution, scalability, workload distribution, and asynchronous I/O.
- **Part E — Epidemiological Analysis:** Investigation of how infection rates, vaccination, masks, lockdown policies, and spatial interactions affect the progression of an epidemic.

The project combines systems programming, concurrency, performance analysis, and epidemiological modelling. It demonstrates how a computational model can be developed incrementally, optimized, and then used to study the behaviour of an epidemic under different conditions.

---

## 2. Part A — Sequential Pandemic Simulation

### 2.1 Objective

The objective of Part A was to develop a functional sequential pandemic simulation. This implementation acts as the reference version against which the parallel and asynchronous implementations can be evaluated.

The simulation represents a population using a two-dimensional grid. Every cell represents one individual, and each individual can occupy one of five epidemiological states:

- **Susceptible (S):** An individual who has not contracted the disease and can become infected.
- **Infected (I):** An individual who is currently carrying and potentially transmitting the disease.
- **Recovered (R):** An individual who has recovered and is no longer susceptible within the model.
- **Vaccinated (V):** An individual who has been vaccinated and is protected from infection in this simulation.
- **Dead (D):** An individual who has died due to the disease.

The simulation advances in discrete time steps, where each time step represents one day.

### 2.2 Population Representation

The population is stored as a one-dimensional vector representing a two-dimensional grid. A cell at row `r` and column `c` is mapped to an index in the vector.

This representation provides efficient memory access and avoids the additional overhead of nested containers.

Each individual is represented by a `Person` structure containing:

- The current epidemiological state.
- The number of days the individual has remained infected.

The infected-day counter is used to determine when an infected individual should transition to either the recovered or dead state.

The simulation also maintains a `Config` structure containing the parameters required to control the experiment. These include:

- Grid size and total population.
- Number of simulation days.
- Initial number of infected individuals.
- Infection duration.
- Infection transmission rate.
- Probability of death.
- Vaccination fraction.
- Mask effectiveness factor.
- Lockdown reduction factor.
- Lockdown activation and deactivation thresholds.
- Random seed.
- Threading and asynchronous execution settings.
- Output file location.

This configuration-based design makes it possible to execute different experiments without changing the source code.

### 2.3 Initialization of the Population

At the beginning of the simulation, all individuals are initialized according to the configured vaccination and infection parameters.

The initialization procedure follows these steps:

1. Calculate the total population from the grid dimensions.
2. Generate a list of all cell indices.
3. Shuffle the indices using a seeded random number generator.
4. Assign the required number of individuals to the vaccinated state.
5. Assign the initial infected individuals from the remaining population.
6. Leave all other individuals in the susceptible state.

Randomized initialization avoids placing all infected or vaccinated individuals in a fixed region of the grid.

The implementation also validates that the number of initially vaccinated and infected individuals does not exceed the total population.

### 2.4 Infection Transmission Model

The simulation uses a Moore neighbourhood, meaning that each individual can interact with up to eight surrounding neighbours:

- The individual directly above.
- The individual directly below.
- The individual to the left.
- The individual to the right.
- The four diagonal neighbours.

Individuals outside the grid are ignored. The grid does not wrap around at its boundaries.

For a susceptible individual, the number of infected neighbours is counted. Let:

- \(\beta\) represent the base infection rate.
- \(m\) represent the mask effectiveness factor.
- \(\lambda\) represent the lockdown reduction factor.
- \(k\) represent the number of infected neighbours.

The effective infection rate is:

\[
\beta_{\text{eff}} = \beta \times m \times \lambda
\]

The probability that a susceptible individual becomes infected is calculated as:

\[
P(\text{infection}) = 1-(1-\beta_{\text{eff}})^k
\]

This formulation accounts for exposure from multiple infected neighbours. As the number of infected neighbours increases, the probability of infection also increases.

If no infected neighbours are present, the infection probability is zero.

### 2.5 Recovery and Death

Every infected individual has an associated infection duration.

At each simulation step, the infected-day counter is incremented. Once the individual reaches the configured infection duration, the simulation determines whether the individual dies or recovers.

The death probability is controlled by the parameter \(\mu\).

- If the generated random value is below the configured death probability, the individual transitions to the dead state.
- Otherwise, the individual transitions to the recovered state.

Recovered and dead individuals do not participate in further infection transmission.

### 2.6 Daily Simulation Update

The simulation uses two separate grid buffers:

- The **current grid** stores the population state at the beginning of the day.
- The **next grid** stores the population state after the daily update.

All infection decisions are calculated using the current grid. The resulting states are written into the next grid.

After the complete grid has been processed, the buffers are exchanged.

This approach is important because it prevents the result of updating one individual from affecting the infection calculation of another individual during the same day. Every individual therefore observes a consistent population state for that time step.

The daily simulation procedure consists of:

1. Reading the current population state.
2. Counting infected neighbours for susceptible individuals.
3. Calculating infection probabilities.
4. Updating infected individuals' infection durations.
5. Applying recovery or death transitions.
6. Writing the updated states into the next grid.
7. Swapping the current and next grids.
8. Counting the number of individuals in each state.
9. Recording the daily results.

### 2.7 Deterministic Random Number Generation

Reproducibility is important when comparing sequential and parallel implementations.

The project uses a deterministic random-number generation approach based on a seeded hash function. The generated random value depends on the simulation seed, day, cell index, and random stream.

Conceptually, the random value is determined by:

\[
R = f(\text{seed},\text{day},\text{cell},\text{stream})
\]

Different random streams are used for different decisions, such as infection and death.

This design makes random decisions independent of the order in which cells are processed. Consequently, changing the execution order or distributing cells among different threads does not inherently change the random values assigned to those cells.

This is particularly useful for verifying the correctness of parallel implementations.

### 2.8 Lockdown Mechanism

The simulation supports an adaptive lockdown mechanism.

Two thresholds are configured:

- \(\theta_{\text{on}}\): The threshold at which lockdown is activated.
- \(\theta_{\text{off}}\): The threshold at which lockdown is deactivated.

The simulation monitors the infected fraction of the population.

When the infected fraction reaches the activation threshold, lockdown is enabled. When the infected fraction falls below the deactivation threshold, lockdown is disabled.

The effective infection rate is reduced when lockdown is active.

Using separate activation and deactivation thresholds avoids frequent switching between lockdown states when the infection fraction fluctuates around a single threshold.

### 2.9 Output and Validation

The simulation records daily epidemiological information in a CSV file.

The output includes:

- Simulation day.
- Number of susceptible individuals.
- Number of infected individuals.
- Number of recovered individuals.
- Number of vaccinated individuals.
- Number of dead individuals.
- Effective infection rate.
- Population checksum.

The checksum provides an additional way to compare simulation states and verify that different execution strategies produce consistent results.

The implementation also includes command-line configuration and validation. Parameters such as infection rate, death probability, vaccination fraction, grid dimensions, and thread counts are checked before execution.

### 2.10 Part A Outcome

Part A established the core simulation model and provided the sequential reference implementation.

The major outcomes were:

- A working grid-based pandemic simulation.
- Five epidemiological population states.
- Local infection transmission through eight neighbours.
- Recovery and death transitions.
- Vaccination, masks, and adaptive lockdown support.
- Deterministic random-number generation.
- CSV output and population checksums.
- Command-line parameter configuration and input validation.

This implementation provided the foundation for the concurrency and performance improvements introduced in the subsequent parts.

---

## 3. Part B — Parallel Simulation

### 3.1 Objective

The objective of Part B was to reduce the execution time of the pandemic simulation by using multiple CPU threads.

In the sequential implementation, the simulation processes the population one cell at a time. For a large grid, this creates a significant computational workload.

However, the daily update of one cell can be performed independently of the update of another cell, provided that all threads read from the same current grid and write to separate locations in the next grid.

This property makes the simulation suitable for data parallelism.

### 3.2 Parallelization Strategy

The parallel implementation distributes the grid-processing workload among multiple threads.

Instead of one thread processing every individual, the population is divided into smaller work units. Each thread processes an assigned portion of the grid.

The implementation investigates different approaches to thread management and workload distribution.

### 3.3 Thread-Per-Day Approach

The first parallel approach creates multiple threads to process different portions of the grid during a simulation day.

The grid is divided into regions, and each thread is responsible for updating its assigned region.

The main steps are:

1. Divide the grid into independent work units.
2. Create the required worker threads.
3. Assign a portion of the grid to each thread.
4. Allow all threads to read from the current grid.
5. Let each thread write updated states into its assigned locations in the next grid.
6. Wait for all threads to finish.
7. Swap the current and next grids.

The synchronization point at the end of each day ensures that the next simulation day does not begin before all updates for the current day have completed.

### 3.4 Thread Pool Implementation

Creating and destroying threads repeatedly can introduce unnecessary overhead.

To reduce this overhead, the project also implements a thread pool.

A thread pool creates a fixed number of worker threads that remain available to execute submitted tasks.

The main components of the thread pool include:

- A collection of worker threads.
- A queue of pending tasks.
- A mutex to protect access to shared task data.
- A condition variable to notify workers when tasks become available.
- A mechanism for waiting until submitted work has completed.

The thread pool provides a reusable mechanism for distributing computation without repeatedly creating new threads.

This approach is particularly useful when the simulation is executed for many days or when multiple work units must be processed repeatedly.

### 3.5 Workload Partitioning Strategies

Different partitioning methods were considered to understand how the arrangement of work affects parallel performance.

**Block partitioning**

- The grid is divided into contiguous blocks.
- Each thread processes one or more complete blocks.
- This approach provides relatively good memory locality.
- It reduces the need for threads to move between distant memory regions.

**Row partitioning**

- Each thread is assigned a range of rows.
- The grid is divided horizontally.
- This method is straightforward to implement and provides a simple workload distribution strategy.

**Interleaved partitioning**

- Work is distributed in an interleaved manner.
- Threads process alternating portions of the grid.
- This approach changes how work is distributed across threads and can be useful for investigating workload balance.

The performance of a partitioning strategy depends on the workload, memory-access patterns, thread count, and synchronization overhead.

### 3.6 Avoiding Data Races

Parallel execution introduces the possibility of data races if multiple threads access shared data without proper coordination.

The simulation avoids this problem through the use of separate current and next grids.

The current grid is read-only during a daily update. Each cell in the next grid has one designated update operation.

As a result:

- Multiple threads can safely read neighbouring cells.
- Threads do not modify the current grid.
- Each output cell is updated independently.
- The grid swap happens only after all workers have completed their tasks.

This design minimizes the need for fine-grained locking inside the computational loop.

### 3.7 Correctness Verification

Parallel correctness was verified by comparing the results of different execution configurations.

The verification process tested combinations of:

- Thread counts.
- Partitioning strategies.
- Simulation parameters.
- Random seeds.

The deterministic random-number generation mechanism helped ensure that the same logical cell and simulation day use the same random inputs across execution strategies.

The parallel verification tests reported:

**42 out of 42 tested combinations passed.**

This result supports the correctness of the tested parallel configurations.

### 3.8 Performance Benchmark

A benchmark was performed using a grid size of \(1000 \times 1000\) and 100 simulation days.

The measured execution times were:

- Sequential implementation: **2.943 seconds**.
- Best tested parallel configuration: **1.267 seconds**.
- Parallel configuration: thread pool with block partitioning and eight threads.

The speedup is calculated as:

\[
\text{Speedup} =
\frac{T_{\text{sequential}}}{T_{\text{parallel}}}
\]

Therefore:

\[
\text{Speedup} =
\frac{2.943}{1.267}
\approx 2.324
\]

The best tested parallel configuration achieved approximately **2.324× speedup** over the sequential implementation.

This indicates that distributing the computational workload across threads substantially reduced execution time for the tested workload.

### 3.9 Interpretation

The benchmark demonstrates that parallel execution can improve performance, but the speedup is not necessarily equal to the number of threads.

Several factors influence parallel performance:

- Thread creation and scheduling overhead.
- Synchronization between simulation days.
- Memory bandwidth limitations.
- Cache locality.
- Workload distribution.
- The number of physical CPU cores.
- Operating-system scheduling.

The thread pool and block partitioning combination provided the best measured result among the tested configurations.

### 3.10 Part B Outcome

Part B successfully extended the sequential simulation with parallel execution.

The major outcomes were:

- Multiple approaches to distributing grid-processing work.
- A reusable thread pool.
- Row, block, and interleaved partitioning strategies.
- Safe parallel updates using separate grid buffers.
- Deterministic behaviour across tested configurations.
- Successful verification of 42 test combinations.
- A measured speedup of approximately 2.324× for the reported benchmark.

---

## 4. Part C — Asynchronous Execution

### 4.1 Objective

The objective of Part C was to introduce asynchronous execution to reduce waiting time and improve the overall utilization of computational resources.

A simulation does not only perform mathematical calculations. It may also need to write snapshots, save CSV results, and execute multiple independent experiments.

These operations can introduce additional overhead, especially when file operations occur frequently.

Part C addresses this problem through two main components:

- Asynchronous snapshot writing.
- Asynchronous parameter sweeps.

### 4.2 C1 — Asynchronous Snapshot Writer

The purpose of the asynchronous snapshot writer is to separate simulation computation from snapshot file writing.

In a fully synchronous approach, the simulation must wait for the snapshot to be written before continuing. This can reduce performance when file operations are relatively slow.

The asynchronous approach moves snapshot writing into a separate execution path.

The main workflow is:

1. The simulation advances to a configured snapshot day.
2. The simulation prepares the snapshot data.
3. The snapshot is submitted to an asynchronous writer.
4. The simulation continues with subsequent computation.
5. The writer saves the submitted snapshot independently.
6. The simulation waits when necessary to respect the configured limit on pending writes.
7. All outstanding writes are completed before the program exits.

This design allows computation and I/O to overlap.

### 4.3 Managing Snapshot Data Safely

Asynchronous execution requires careful handling of memory.

A snapshot writer must not access a grid buffer while the simulation is modifying or reusing that same buffer.

Therefore, the snapshot data must remain valid until the writing operation completes.

The implementation uses an independent snapshot representation for asynchronous processing. This prevents the writer from observing partially updated simulation data.

The writer also uses a configurable limit on the number of pending operations.

The `maxPending` parameter controls how many asynchronous snapshot operations can remain outstanding.

This prevents unlimited accumulation of pending work and helps control memory consumption.

### 4.4 Simulating I/O Latency

The implementation supports configurable artificial I/O latency.

This makes it possible to investigate how asynchronous execution behaves when snapshot writing takes additional time.

By varying the simulated latency, the experiment can examine whether the simulation is able to perform useful computation while a snapshot is being written.

The main performance considerations are:

- Time required to prepare snapshot data.
- Time spent writing files.
- Number of pending snapshot operations.
- Time spent waiting for the writer.
- Memory required to retain pending snapshots.

Asynchronous writing is most useful when there is sufficient independent computation to overlap with the I/O operation.

### 4.5 C2 — Asynchronous Parameter Sweep

The second component of Part C implements parameter sweeps.

A parameter sweep executes multiple independent simulations using different parameter combinations.

For example, experiments may use different:

- Infection rates.
- Vaccination fractions.
- Random seeds.

The implementation constructs combinations from the configured parameter lists.

If there are \(B\) infection-rate values, \(V\) vaccination values, and \(S\) random seeds, the total number of independent runs is:

\[
N_{\text{runs}} = B \times V \times S
\]

Each run is represented by a `SweepRun` structure containing:

- A unique run identifier.
- Infection rate.
- Vaccination fraction.
- Random seed.

The parameter sweep executes these independent runs concurrently.

### 4.6 Worker-Based Execution

The parameter sweep uses a worker-based execution model.

The main components are:

- A collection of worker threads.
- A shared list of simulation jobs.
- A synchronized mechanism for assigning jobs.
- A result queue.
- A coordinator that collects completed results.

Each worker repeatedly obtains an available job, executes the simulation, and places the resulting summary into the completion queue.

The coordinator waits for completed results and writes them to the output CSV file.

This design allows simulations to finish in any order. The coordinator records each result as it becomes available.

### 4.7 Simulation Summary

Each parameter-sweep run produces a `RunSummary` containing important epidemiological measurements:

- Peak infected fraction.
- Day on which the peak occurs.
- Total number of infected individuals.
- Total deaths.
- Number of simulation days completed.

The completion structure also stores the original run configuration and any error information.

This makes it possible to associate each result with the parameters that produced it.

### 4.8 Synchronization and Error Handling

The parameter sweep uses mutexes and condition variables to coordinate workers and the result collector.

The mutex protects shared data structures, such as job assignment and result queues.

The condition variable allows the coordinator to wait for new results without continuously polling the queue.

The implementation also supports a configurable execution time limit.

A deadline is established for the sweep. Workers stop obtaining additional jobs when the deadline has been reached, while the coordinator collects completed results and records failures or missing runs.

Worker threads are joined before the sweep function exits.

This ensures that the program does not terminate while worker threads are still executing.

### 4.9 Avoiding Nested Parallelism

An important design decision is that each parameter-sweep worker runs an individual simulation using the sequential simulation routine.

The parameter sweep parallelizes across independent simulation runs rather than creating a second layer of parallel threads inside every simulation.

This avoids unnecessary nested parallelism.

Nested parallelism can cause excessive thread creation, CPU oversubscription, scheduling overhead, and unpredictable performance.

By parallelizing at the outer level, the implementation provides a simpler and more controlled execution model.

### 4.10 Part C Outcome

Part C introduced asynchronous functionality in two areas:

- Snapshot writing can take place independently of the main simulation computation.
- Independent simulation runs can be executed concurrently during parameter sweeps.

The implementation also introduced bounded pending snapshot operations, synchronized result collection, worker coordination, execution time limits, and per-run error reporting.

These features make the simulation more suitable for large experimental workloads.

---

## 5. Part D — Performance Study of Parallel and Asynchronous Simulation

### 5.1 Objective

The objective of Part D was to experimentally evaluate the performance of the simulation implementations developed in the previous parts.

The main focus was to understand how parallelism and asynchronous execution affect execution time, scalability, workload distribution, and resource utilization.

Rather than assuming that adding more threads always improves performance, the study investigates the conditions under which parallel execution provides meaningful benefits.

The experiments were organized into five major areas.

### 5.2 Experimental Hardware

The experiments were conducted on a system with the following processor configuration:

- Processor: Intel Core i5-10300H.
- Processor frequency: 2.50 GHz.
- Physical CPU cores: 4.
- Logical CPUs: 8.
- L1 data cache: 128 KiB.
- L1 instruction cache: 128 KiB.
- L2 cache: 1 MiB.
- L3 cache: 8 MiB.

The distinction between physical cores and logical CPUs is important when interpreting parallel performance.

The system has four physical cores and supports eight logical CPUs through simultaneous multithreading. However, eight logical CPUs do not necessarily provide twice the computational capacity of four physical cores.

Performance also depends on cache behaviour, memory bandwidth, CPU scheduling, and synchronization overhead.

### 5.3 D1 — Strong Scaling

Strong scaling measures how execution time changes when the number of processing threads increases while the total problem size remains fixed.

In this experiment, the simulation workload is kept constant, and the number of worker threads is varied.

The purpose is to determine whether the same amount of computational work can be completed faster by assigning it to more threads.

The main performance metrics are:

**Execution time**

The total time required to complete the simulation.

**Speedup**

\[
S(p) = \frac{T(1)}{T(p)}
\]

Here, \(T(1)\) represents the execution time using one thread, while \(T(p)\) represents execution time using \(p\) threads.

**Parallel efficiency**

\[
E(p) = \frac{S(p)}{p}
\]

Efficiency indicates how effectively the available threads contribute to reducing execution time.

As the number of threads increases, execution time may decrease initially. However, the improvement may become smaller because of synchronization, scheduling, and hardware limitations.

The processor used in this study has four physical cores and eight logical CPUs. Consequently, increasing the thread count beyond the available physical cores may provide diminishing returns.

### 5.4 D2 — Weak Scaling

Weak scaling investigates how performance changes when the problem size increases proportionally with the number of processing resources.

Unlike strong scaling, the objective is not to solve the same problem faster. Instead, the objective is to determine whether the system can maintain approximately consistent execution time as both the workload and available parallel resources increase.

The experiment helps evaluate whether the parallel simulation can handle larger populations efficiently.

Important considerations include:

- Growth in the number of grid cells.
- Distribution of work across threads.
- Memory consumption.
- Cache behaviour.
- Synchronization overhead.
- Ability to maintain workload balance.

Weak scaling is especially relevant for pandemic simulation because larger populations may require substantially more computational work.

### 5.5 D3 — Work-Unit Size and Partitioning

This experiment investigates the effect of workload partitioning on parallel performance.

The simulation supports different methods of distributing grid cells among worker threads.

The evaluated strategies include row-based, block-based, and interleaved partitioning.

Each approach has different characteristics.

**Row-based partitioning**

- Assigns contiguous rows to workers.
- Is simple to implement.
- Provides predictable division of the grid.
- Can offer good memory locality because adjacent cells are processed together.

**Block-based partitioning**

- Divides the grid into larger contiguous regions.
- Provides localized memory access.
- Can reduce unnecessary movement between distant memory regions.
- Allows work to be organized into manageable chunks.

**Interleaved partitioning**

- Distributes work in an alternating pattern.
- Changes the way individual cells are assigned to workers.
- Provides a useful comparison against contiguous partitioning.

The experiment demonstrates why workload distribution is an important part of parallel program design.

Even when two approaches perform the same number of logical operations, their execution times may differ because of memory locality, cache utilization, and scheduling behaviour.

### 5.6 D4 — Parallelism Within One Simulation Versus Across Independent Simulations

This experiment compares two different levels of parallelism.

The first approach parallelizes a single simulation by dividing the population grid among worker threads.

The second approach parallelizes multiple independent simulations through the parameter-sweep implementation.

These approaches address different computational workloads.

**Parallelism within one simulation**

- Divides the grid-processing work among threads.
- Accelerates an individual simulation run.
- Requires synchronization between simulation days.
- Is useful when one simulation contains a large population.

**Parallelism across independent simulations**

- Executes multiple independent parameter combinations concurrently.
- Does not require synchronization between separate simulation runs.
- Is useful for scientific experiments and parameter exploration.
- Can keep multiple workers occupied with independent tasks.

The parameter-sweep approach is particularly suitable when many combinations of infection rates, vaccination fractions, and random seeds must be evaluated.

The comparison highlights that parallelism should be selected according to the structure of the workload rather than applied in the same way to every task.

### 5.7 D5 — Hiding I/O Time Using Asynchronous Snapshot Writing

This experiment investigates whether snapshot writing can be overlapped with simulation computation.

In synchronous execution, the simulation may need to wait until a snapshot is completely written.

In asynchronous execution, snapshot writing is handled independently while the simulation continues processing later days.

The experiment considers:

- Snapshot frequency.
- Artificial I/O latency.
- Number of pending writes.
- Time spent waiting for asynchronous operations.
- Total simulation execution time.

The purpose is to determine whether asynchronous execution reduces the effect of I/O operations on the main computational workload.

The benefit depends on the relationship between computation time and I/O time.

If the simulation performs enough computation while the writer is active, asynchronous execution can hide part of the I/O latency.

However, asynchronous execution does not eliminate the cost of writing data. Snapshot preparation, memory allocation, file-system activity, and waiting for outstanding operations can still affect performance.

### 5.8 Overall Interpretation of Part D

The performance study demonstrates that parallel and asynchronous techniques solve different performance problems.

Parallel processing is useful when a large computational workload can be divided into independent tasks.

Asynchronous execution is useful when independent activities, such as file writing, can overlap with computation.

The experiments also emphasize that performance depends on the hardware and the structure of the workload.

Adding more threads does not guarantee proportional speedup because of:

- Synchronization overhead.
- Thread scheduling.
- Cache limitations.
- Memory bandwidth.
- Workload imbalance.
- Physical CPU resource constraints.

Similarly, asynchronous I/O provides the greatest benefit when there is enough independent computation to overlap with file operations.

### 5.9 Part D Outcome

Part D provided a systematic performance-evaluation framework for the project.

It investigated:

- Strong scaling.
- Weak scaling.
- Work-unit size and partitioning.
- Parallelism within a simulation.
- Parallelism across independent simulations.
- Asynchronous snapshot writing.

The study used execution time, speedup, efficiency, and workload behaviour to evaluate the implementation.

The results from Part B also provide a concrete performance reference: the reported \(1000 \times 1000\), 100-day benchmark achieved approximately 2.324× speedup using a thread pool, block partitioning, and eight threads.

The other Part D experiment measurements should be interpreted using the original benchmark output and plots submitted with the experiment data.

---

## 6. Part E — Epidemiological Analysis

### 6.1 Objective

The objective of Part E was to use the completed simulation to investigate epidemiological behaviour under different conditions.

The experiments focus on how infection rates, vaccination coverage, masks, and lockdown policies influence:

- Peak infection levels.
- Time of peak infection.
- Total attack rate.
- Epidemic duration.
- Probability of a significant outbreak.
- Number of deaths.

The study also compares the spatial simulation with a traditional deterministic SIR model.

### 6.2 Baseline Simulation Configuration

The primary epidemiological experiments used the following configuration:

- Grid size: \(500 \times 500\).
- Total population: 250,000 individuals.
- Initial infected population: 10 individuals.
- Infection duration: 7 days.
- Default infection rate: 0.08.
- Death probability: 0.01.
- Maximum experiment duration: 1,000 days.
- Number of independent random seeds for parameter experiments: 10.

The simulation uses the same five population states: susceptible, infected, recovered, vaccinated, and dead.

The eight-neighbour infection model allows the disease to spread locally across the population grid.

### 6.3 Epidemiological Metrics

The analysis uses several measurements to describe epidemic behaviour.

**Peak infected population**

The maximum number of simultaneously infected individuals during a simulation.

This metric represents the highest disease burden experienced at one time.

**Peak day**

The day on which the infected population reaches its maximum.

This helps determine how quickly an epidemic develops.

**Attack rate**

The attack rate measures the proportion of the population that has been infected.

It is calculated as:

\[
\text{Attack Rate} =
\frac{R + D + I}{N} \times 100
\]

Here, \(R\) represents recovered individuals, \(D\) represents deaths, \(I\) represents currently infected individuals, and \(N\) represents the total population.

Vaccinated individuals are not included in the numerator unless they become infected, which the current simulation does not permit.

**Outbreak probability**

For the experiments, an outbreak is operationally defined as a run with an attack rate of at least 1%.

For a population of 250,000, this corresponds to at least 2,500 infected individuals.

The outbreak probability is calculated as the proportion of independent runs that cross this threshold.

**Epidemic duration**

The number of days required for the simulation to reach its termination condition.

For experiments that do not finish within the configured 1,000-day limit, the run is treated as incomplete.

### 6.4 Baseline Results

The baseline simulation was executed using seed 42.

The observed results were:

- Total population: 250,000.
- Initial infected individuals: 10.
- Epidemic duration: 852 days.
- Peak infected population: 5,351.
- Peak day: 131.
- Final susceptible population: 2,780.
- Final recovered population: 244,629.
- Final dead population: 2,591.
- Final infected population: 0.

The results show that the epidemic spread through a large proportion of the population.

The peak infected population was much smaller than the total population because the simulation uses local spatial interactions rather than assuming that every individual can interact with every other individual.

The epidemic also developed over a relatively long period, with the peak occurring on day 131.

### 6.5 Experiment 1 — Effect of Infection Rate

The first experiment investigated how changing the base infection rate affects epidemic progression.

The infection rate was varied across:

\[
0.02,\ 0.04,\ 0.06,\ 0.08,\ 0.10,\ 0.12
\]

Ten independent random seeds were used for each infection-rate setting.

The results showed a strong relationship between infection rate and epidemic severity.

At an infection rate of 0.02:

- Average peak infected population: 22.3.
- Average peak day: 6.0.
- Average attack rate: 0.02%.
- Outbreak probability: 0%.

At an infection rate of 0.04:

- Average peak infected population: 626.9.
- Average peak day: 578.
- Average attack rate: 20.66%.
- Outbreak probability: 100%.

At an infection rate of 0.06:

- Average peak infected population: 4,473.1.
- Average peak day: 237.6.
- Average attack rate: 94.40%.
- Outbreak probability: 100%.

At an infection rate of 0.08:

- Average peak infected population: 6,178.
- Average peak day: 180.
- Average attack rate: 98.88%.
- Outbreak probability: 100%.

At an infection rate of 0.10:

- Average peak infected population: 7,471.2.
- Average peak day: 145.
- Average attack rate: 99.69%.
- Outbreak probability: 100%.

At an infection rate of 0.12:

- Average peak infected population: 8,405.8.
- Average peak day: 130.6.
- Average attack rate: 99.91%.
- Outbreak probability: 100%.

The results demonstrate that increasing the infection rate leads to a larger epidemic peak and a higher attack rate.

The peak also tends to occur earlier at higher infection rates.

At low infection rates, transmission is less effective, and the epidemic may remain small or develop very slowly.

An important observation is the transition between infection rates 0.02 and 0.04. The outbreak probability changes from 0% to 100% under the defined 1% outbreak threshold.

Some runs at infection rates 0.04 and 0.06 did not complete within the 1,000-day experiment limit. Their duration-related measurements must therefore be interpreted with this limitation in mind.

### 6.6 Experiment 2 — Effect of Vaccination Coverage

The second experiment investigated how vaccination coverage affects the spread of infection.

Vaccination fractions were varied from 0% to 90%, in increments of 10%.

Ten independent seeds were used for each vaccination setting.

The results showed that vaccination substantially reduces epidemic severity.

At 0% vaccination:

- Average peak infected population: 6,178.
- Average attack rate: 98.88%.
- Outbreak probability: 100%.

At 10% vaccination:

- Average peak infected population: 5,079.
- Average attack rate: 87.90%.
- Outbreak probability: 100%.

At 20% vaccination:

- Average peak infected population: 3,662.4.
- Average attack rate: 75.59%.
- Outbreak probability: 100%.

At 30% vaccination:

- Average peak infected population: 1,585.9.
- Average attack rate: 51.16%.
- Outbreak probability: 100%.

At 40% vaccination:

- Average peak infected population: 91.6.
- Average attack rate: 0.48%.
- Outbreak probability: 0%.

At 50% vaccination:

- Average peak infected population: 42.4.
- Average attack rate: 0.08%.
- Outbreak probability: 0%.

At 60% vaccination:

- Average peak infected population: 29.2.
- Average attack rate: 0.03%.
- Outbreak probability: 0%.

At 70% vaccination:

- Average peak infected population: 21.6.
- Average attack rate: 0.01%.
- Outbreak probability: 0%.

At 80% vaccination:

- Average peak infected population: 16.8.
- Average attack rate: 0.01%.
- Outbreak probability: 0%.

At 90% vaccination:

- Average peak infected population: 13.2.
- Average attack rate: 0.01%.
- Outbreak probability: 0%.

The results show a substantial reduction in epidemic severity as vaccination coverage increases.

The most important observation is the change between 30% and 40% vaccination.

At 30% vaccination, all ten runs crossed the defined outbreak threshold. At 40% vaccination, none of the ten runs crossed it.

Therefore, for this particular simulation configuration, the observed outbreak-suppression transition lies between 30% and 40% vaccination coverage.

This should not be interpreted as a universal vaccination threshold. It depends on the infection rate, population arrangement, infection duration, neighbourhood structure, random seeds, and the other model parameters.

### 6.7 Experiment 3 — Effect of Masks and Lockdown

The third experiment investigated the effect of non-pharmaceutical interventions.

The experiments considered different combinations of mask effectiveness and lockdown reduction factors.

The effective infection rate is calculated using:

\[
\beta_{\text{eff}} = \beta \times m \times \lambda
\]

The tested configurations included:

**No intervention**

- Mask factor: 1.0.
- Lockdown factor: 1.0.
- Average peak infected population: 6,178.
- Average attack rate: 98.88%.
- Outbreak probability: 100%.

This configuration acts as the reference for comparing intervention strategies.

**Mild mask intervention**

- Mask factor: 0.8.
- Lockdown factor: 1.0.
- Average peak infected population: 4,840.9.
- Average peak reduction: 21.64%.
- Average attack rate: 96.06%.
- Outbreak probability: 100%.

The mild mask intervention reduces the peak infected population, although it does not prevent the outbreak.

**Strong mask intervention**

- Mask factor: 0.5.
- Lockdown factor: 1.0.
- Average peak infected population: 626.9.
- Average peak reduction: 89.85%.
- Average attack rate: 20.66%.
- Outbreak probability: 100%.

The stronger mask setting substantially reduces the peak and attack rate. However, all runs still cross the defined outbreak threshold.

**Mild lockdown intervention**

- Mask factor: 1.0.
- Lockdown factor: 0.7.
- Average peak infected population: 5,036.8.
- Average peak reduction: 18.47%.
- Average attack rate: 95.45%.
- Outbreak probability: 100%.

The mild lockdown setting produces a reduction in epidemic severity, but its effect is limited under the tested configuration.

**Strong lockdown intervention**

- Mask factor: 1.0.
- Lockdown factor: 0.3.
- Average peak infected population: 5,234.9.
- Average peak reduction: 15.27%.
- Average attack rate: 94.72%.
- Outbreak probability: 100%.

The result indicates that the adaptive lockdown configuration did not suppress the epidemic as effectively as the stronger mask intervention in this experiment.

**Combined mask and lockdown intervention**

- Mask factor: 0.5.
- Lockdown factor: 0.3.
- Average peak infected population: 104.5.
- Average peak reduction: 98.31%.
- Average attack rate: 2.20%.
- Outbreak probability: 80%.

The combined intervention produces the largest reduction in peak infections among the tested configurations.

However, eight out of ten runs still cross the defined 1% outbreak threshold.

The combined experiment also uses lower lockdown activation and deactivation thresholds than the regular configuration:

- Lockdown activation threshold: 0.0004.
- Lockdown deactivation threshold: 0.0001.

This is an important experimental detail because the lockdown policy is adaptive. The timing at which lockdown is activated can influence how much transmission occurs before the intervention takes effect.

### 6.8 Interpretation of Intervention Results

The intervention experiments demonstrate that reducing the effective infection rate can substantially reduce epidemic severity.

The results also show that reducing the peak infected population and preventing an outbreak are not the same objective.

For example, the combined intervention reduces the average peak infected population by approximately 98.31%, yet the outbreak probability remains 80% under the defined threshold.

This means that an intervention may greatly reduce the maximum number of simultaneously infected individuals while still allowing the total number of infections over the full epidemic to exceed the outbreak threshold.

The results suggest that combining interventions can be more effective than relying on a single intervention, although the effectiveness depends on how the interventions are configured and when they are applied.

### 6.9 Experiment 4 — Comparison with the Deterministic SIR Model

The final epidemiological experiment compares the spatial simulation with a traditional deterministic SIR model.

The SIR model divides the population into three compartments:

- Susceptible.
- Infected.
- Recovered.

Unlike the grid-based simulation, the traditional SIR model assumes a well-mixed population. It does not explicitly represent the spatial arrangement of individuals or local neighbourhood interactions.

The SIR model uses the following parameters:

- Population: 250,000.
- Initial infected individuals: 10.
- Infection rate: 0.64.
- Recovery rate: \(1/7 \approx 0.142857\).

The infection rate of 0.64 is an approximate mean-field mapping from the spatial model's infection rate of 0.08 and its eight-neighbour structure:

\[
\beta_{\text{SIR}} = 8 \times 0.08 = 0.64
\]

This is an approximate comparison rather than an exact calibration between the two models.

The deterministic SIR model produced the following results:

- Peak infected population: approximately 119,543.37.
- Peak day: 27.
- Epidemic duration: 202 days.
- Final susceptible population: approximately 1,746.27.
- Final recovered population: approximately 248,253.73.

The spatial simulation, by comparison, produced a peak infected population of 5,351 on day 131 in the baseline seed-42 run.

The SIR model therefore predicts a much larger and earlier infection peak.

The ratio of the SIR peak to the spatial simulation peak is approximately:

\[
\frac{119543.37}{5351} \approx 22.34
\]

The deterministic SIR peak is approximately 22.34 times the spatial simulation's baseline peak.

### 6.10 Why the Two Models Differ

The difference between the models is primarily due to their underlying assumptions.

The spatial model:

- Represents individuals on a two-dimensional grid.
- Allows infection only through nearby neighbours.
- Includes local spatial constraints.
- Uses stochastic infection and death decisions.
- Represents vaccination and adaptive interventions.
- Allows the epidemic to progress through local clusters.

The deterministic SIR model:

- Assumes a well-mixed population.
- Does not represent individual locations.
- Uses population-level differential equations.
- Does not explicitly model local neighbourhood restrictions.
- Does not include the same vaccination, mask, lockdown, and death compartments in this comparison.

Because the SIR model assumes that individuals can interact throughout the population, it predicts much faster and broader transmission.

The spatial model restricts transmission to local neighbours, which slows down the spread and changes the timing and magnitude of the epidemic peak.

The comparison demonstrates the importance of modelling assumptions when interpreting epidemiological predictions.

### 6.11 Part E Outcome

Part E used the completed simulation to investigate the effects of epidemiological parameters and interventions.

The major findings were:

- Increasing the infection rate substantially increases the average peak infected population and attack rate.
- Higher infection rates cause the epidemic peak to occur earlier.
- Vaccination coverage significantly reduces epidemic severity.
- For the tested configuration, the observed outbreak-suppression transition lies between 30% and 40% vaccination.
- Strong mask effectiveness reduces peak infections substantially.
- The combined mask and lockdown intervention produces the largest reduction in the average peak among the tested intervention settings.
- A reduced peak does not necessarily mean that the outbreak threshold will not be crossed.
- The spatial simulation produces a much smaller and later infection peak than the approximate deterministic SIR comparison.

---

## 7. Limitations and Assumptions

Although the simulation provides useful insights, several limitations must be considered when interpreting its results.

### 7.1 Simplified Disease Model

The simulation uses a simplified epidemiological model.

It does not represent all real-world disease characteristics, such as:

- Different stages of infection.
- Asymptomatic and symptomatic transmission.
- Hospitalization.
- Age-dependent infection or mortality.
- Waning immunity.
- Reinfection.
- Individual mobility.
- Population movement between locations.

Consequently, the model should be treated as an experimental simulation rather than a direct predictor of real-world epidemic outcomes.

### 7.2 Fixed Spatial Structure

The population is represented by a fixed grid.

Individuals do not move between locations, and infection transmission occurs only through the eight neighbouring cells.

Real populations have complex contact networks, mobility patterns, and social interactions that are not captured by this model.

### 7.3 Simplified Vaccination

Vaccinated individuals are protected from infection in the current implementation.

The model does not include partial vaccine effectiveness, waning immunity, or breakthrough infections.

Therefore, the vaccination-suppression results depend on this simplified assumption.

### 7.4 Experiment Time Limit

Parameter-sweep experiments use a maximum simulation duration of 1,000 days.

Some runs did not complete within this limit.

Measurements from incomplete runs must be interpreted carefully, particularly when comparing epidemic duration.

A run that has not reached its termination condition by the time limit should not be treated as a completed epidemic.

### 7.5 Outbreak Threshold

The outbreak definition uses an attack-rate threshold of 1%.

This is an operational threshold chosen for the experiment. It is not a universal epidemiological definition of an outbreak.

Changing the threshold would change the calculated outbreak probability.

### 7.6 SIR Comparison

The SIR comparison uses an approximate infection-rate mapping based on the eight-neighbour structure.

The two models have different assumptions and are not exactly equivalent.

The SIR model also does not include the same death, vaccination, mask, lockdown, and spatial mechanisms as the grid-based model.

Therefore, the comparison is intended to demonstrate differences between modelling approaches rather than to establish an exact numerical equivalence.

### 7.7 Hardware Dependence of Performance

Parallel performance depends on the hardware used for execution.

The measured speedup is specific to the tested system and workload.

Different processors, memory configurations, operating systems, compiler settings, and background processes may produce different results.

---

## 8. Overall Conclusion

The project successfully developed a sequential pandemic simulation and extended it through parallel and asynchronous execution techniques.

Part A established the epidemiological model using a two-dimensional grid, five population states, local infection transmission, recovery, death, vaccination, masks, and adaptive lockdown.

Part B introduced multithreaded execution through different workload partitioning strategies and a reusable thread pool. The implementation passed 42 tested verification combinations. The benchmark using a \(1000 \times 1000\) grid over 100 days achieved approximately 2.324× speedup with a thread pool, block partitioning, and eight threads.

Part C introduced asynchronous snapshot writing and concurrent parameter sweeps. These features allow file operations to overlap with simulation computation and enable independent epidemiological experiments to execute concurrently.

Part D evaluated the performance characteristics of the system. It examined strong scaling, weak scaling, work-unit partitioning, parallelism within a simulation, parallelism across independent runs, and asynchronous I/O. The study demonstrates that performance improvements depend on workload characteristics, synchronization, memory behaviour, and hardware resources.

Part E used the simulation to investigate the effects of infection rates, vaccination, masks, and lockdown. The experiments showed that higher infection rates increase epidemic severity, vaccination substantially reduces the attack rate, and combined interventions can greatly reduce the peak infected population. The comparison with the deterministic SIR model also demonstrated how spatial constraints and local interactions can produce substantially different epidemic dynamics from a well-mixed population model.

Overall, the project demonstrates the importance of combining correct computational modelling with efficient systems-level implementation. It shows how parallelism and asynchronous execution can improve the ability to run large simulations and repeated experiments, while epidemiological analysis can use those simulations to investigate the effects of different parameters and interventions.

The project also highlights an important distinction: a faster simulation is not automatically a more accurate model. Performance optimization and model validity must be evaluated separately. The implementation must preserve correctness while performance experiments must be interpreted in the context of the hardware, workload, and assumptions of the epidemiological model.

---

## 9. Submission Notes

The consolidated report covers Parts A, B, C, D, and E.

The source code, build configuration, and required project documentation should be maintained in the GitHub repository.

Generated experiment outputs, including CSV result files, raw benchmark data, and plot images, should be submitted separately through the designated course submission platform if required by the assignment.

The report's numerical findings should be read together with the corresponding experiment outputs and plots. In particular, the detailed Part D benchmark measurements should be retained with the original performance experiment data rather than replaced with estimated values.
