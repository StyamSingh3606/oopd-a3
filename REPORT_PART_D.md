# Part D: Performance Study of Parallel and Asynchronous Pandemic Simulation

## 1. Introduction

The objective of Part D is to evaluate the performance of the pandemic simulation under different levels of parallelism and asynchronous execution.

The experiments cover five areas:

- D1: Strong scaling.
- D2: Weak scaling.
- D3: Work-unit size and partitioning.
- D4: Parallelism within a simulation versus across independent simulations.
- D5: Hiding I/O time using asynchronous snapshot writing.

The experiments analyze execution time, speedup, efficiency, workload distribution, and the effect of asynchronous operations.

## 2. Hardware and Software Configuration

All experiments were conducted using the same machine and execution environment.

| Component | Specification |
|---|---|
| Processor | Intel Core i5-10300H @ 2.50 GHz |
| Physical cores | 4 |
| Logical CPUs | 8 |
| L1 data cache | 128 KiB |
| L1 instruction cache | 128 KiB |
| L2 cache | 1 MiB |
| L3 cache | 8 MiB |
| Memory | Approximately 3.7 GiB |
| Compiler | GCC 15.2.0 |
| Language | C++17 |
| Environment | WSL Ubuntu |

The processor has four physical cores and eight logical CPUs through hyperthreading. This distinction is important when interpreting the performance obtained with higher thread counts.

## 3. Performance Metrics

The following metrics are used in the experiments.

**Speedup** measures the improvement in execution time when using multiple threads:

\[
S(p)=\frac{T_1}{T_p}
\]

**Parallel efficiency** measures how effectively the available threads are utilized:

\[
E(p)=\frac{S(p)}{p}
\]

**Amdahl's Law** estimates the maximum speedup in the presence of sequential work:

\[
S(p)=\frac{1}{f+\frac{1-f}{p}}
\]

**Karp-Flatt metric** estimates the effective serial fraction:

\[
e(p)=\frac{\frac{1}{S(p)}-\frac{1}{p}}{1-\frac{1}{p}}
\]

For performance measurements, median execution time and variation across repeated measurements are used to reduce the effect of measurement noise.

---

## 4. D1: Strong Scaling

### Objective

Strong scaling measures how execution time changes when the number of threads increases while the total workload remains fixed.

The experiment used a grid of 4000 × 4000 cells, 200 simulation days, and no snapshots. The number of threads was varied from 1 to 16.

### Results

| Threads | Median Time (seconds) | Speedup |
|---:|---:|---:|
| 1 | 81.969 | 1.000 |
| 2 | 51.022 | 1.607 |
| 4 | 32.696 | 2.507 |
| 8 | 27.262 | 3.007 |
| 16 | 25.940 | 3.160 |

The execution time decreases as the number of threads increases. Using four threads achieves a speedup of approximately 2.51. Increasing the number of threads to eight provides additional improvement, but the efficiency decreases.

At 16 threads, the speedup is approximately 3.16, which is considerably lower than ideal linear scaling.

The Karp-Flatt effective serial fraction was approximately 0.245 for two threads, 0.199 for four threads, 0.237 for eight threads, and 0.271 for sixteen threads. The increase at higher thread counts suggests additional parallel overhead.

### Analysis

The machine has four physical cores and eight logical CPUs. Beyond four threads, hyperthreading allows additional logical execution contexts, but these do not provide the same performance as additional physical cores.

Memory bandwidth, cache behaviour, thread scheduling, and synchronization overhead also limit performance.

The results are consistent with Amdahl's Law: increasing the number of threads eventually provides diminishing returns because some work and overhead cannot be parallelized effectively.

**Conclusion:** Four threads provide a good balance between execution time and efficiency. Using more threads provides smaller additional improvements.

---

## 5. D2: Weak Scaling

### Objective

Weak scaling studies how performance changes when the workload increases with the number of threads.

The experiment maintained approximately one million grid cells per thread. The grid size increased as the thread count increased.

The configurations used were:

- 1 thread: 1000 × 1000 grid.
- 2 threads: 1414 × 1414 grid.
- 4 threads: 2000 × 2000 grid.
- 8 threads: 2828 × 2828 grid.

### Results

| Threads | Median Time (seconds) | Efficiency |
|---:|---:|---:|
| 1 | 5.824 | 100.00% |
| 2 | 6.928 | 84.06% |
| 4 | 9.644 | 60.39% |
| 8 | 15.325 | 38.00% |

### Analysis

Ideally, execution time should remain approximately constant when the workload per thread is fixed. However, the measured execution time increases as the number of threads and total population increase.

Efficiency decreases from 100% with one thread to approximately 38% with eight threads. Maximum observed memory usage was approximately 187 MiB.

This behaviour can be explained using Gustafson's Law, which considers how larger workloads can be handled by increasing the number of processors.

The results indicate that the simulation does not achieve ideal weak scaling. Possible reasons include memory bandwidth limitations, cache misses, increased memory traffic, and thread-management overhead.

**Conclusion:** Increasing the number of threads allows the simulation to process a larger population, but memory-system limitations and parallel overhead reduce efficiency.

---

## 6. D3: Work-Unit Size and Partitioning

D3 investigates how task granularity and partitioning strategies affect execution performance.

### D3.1: Number of Chunks

The number of chunks was varied from the number of physical cores up to 64 times that number.

The best tested configuration used four chunks, with a median execution time of 39.960 seconds.

Using too many small tasks can increase scheduling and task-management overhead. A suitable chunk size provides a balance between workload distribution and overhead.

### D3.2: Partitioning Strategies

Three partitioning methods were compared:

- **Row partitioning:** The grid is divided into horizontal strips.
- **Block partitioning:** The grid is divided into square or rectangular blocks.
- **Interleaved partitioning:** Threads process rows in an interleaved pattern.

Block and interleaved partitioning produced nearly identical performance in the tested configuration. Row partitioning was approximately 16% slower.

The differences can be influenced by memory access patterns, cache locality, and workload distribution.

### D3.3: Thread-Per-Day Versus Thread Pool

The thread-per-day approach creates and joins threads during every simulation day. In contrast, the thread pool creates a fixed group of threads and reuses them across days.

Experiments were conducted for grid sizes ranging from N = 50 to N = 1000.

The thread pool reduces repeated thread creation and destruction overhead. The measured crossover point, where parallel execution becomes faster than sequential execution, lies between N = 500 and N = 1000 in the tested configurations.

For smaller grids, parallel overhead can exceed the computational benefit. Larger grids provide sufficient work to make parallel execution more effective.

**Conclusion:** Task granularity, partitioning strategy, and thread reuse all influence performance. A suitable chunk size and partitioning method help reduce overhead.

---

## 7. D4: Parallelism Within a Run Versus Across Runs

### Objective

This experiment compares using multiple threads within one simulation against running several independent simulations concurrently.

A total of 32 independent simulation runs were executed. The machine has four physical cores, so the tested configurations maintained:

\[
R \times T = 4
\]

where R represents concurrent runs and T represents threads per run.

### Results

| Concurrent Runs | Threads per Run | Total Time (seconds) |
|---:|---:|---:|
| 1 | 4 | 64.70 |
| 2 | 2 | 59.33 |
| 4 | 1 | 54.62 |

### Analysis

Running one simulation with four threads took 64.70 seconds. Running two simulations concurrently with two threads each reduced the time to 59.33 seconds.

The best tested result was obtained by running four independent simulations concurrently, each using one thread. The total execution time was 54.62 seconds.

This suggests that parallelism across independent simulations can be more efficient than parallelizing every individual simulation.

Independent runs do not need to coordinate their internal grid updates with one another. Using fewer threads per simulation can also reduce internal scheduling and synchronization overhead.

**Conclusion:** For workloads involving many independent simulations, parallelism across runs can provide better performance on this machine.

---

## 8. D5: Hiding I/O Time Using Asynchronous Snapshots

### Objective

D5 evaluates whether asynchronous snapshot writing can reduce execution time by overlapping file-writing operations with simulation computation.

Three approaches were compared:

1. No snapshots.
2. Blocking snapshot writing.
3. Asynchronous snapshot writing.

The experiment also varied artificial I/O latency, snapshot interval K, and maximum pending writes Q.

### Implementation

In blocking snapshot writing, the simulation waits until the snapshot has been written before continuing.

In asynchronous snapshot writing:

1. The current grid is copied.
2. The copied grid is submitted to an asynchronous task.
3. The simulation continues while the background task writes the snapshot.
4. If the maximum number of pending writes is reached, the simulation waits for the oldest task.
5. All remaining tasks are awaited before the program exits.

The grid copy is necessary because the simulation continues modifying its own grid. The background writer needs a consistent snapshot that will not change during file writing.

### Results

For an artificial I/O latency of 200 milliseconds and snapshot interval K = 5:

| Configuration | Execution Time (seconds) |
|---|---:|
| No snapshots | 1.416 |
| Blocking snapshots | 8.862 |
| Asynchronous snapshots, Q = 4 | 2.694 |

### Analysis

Blocking snapshot writing significantly increases execution time because the simulation must wait for I/O operations.

Asynchronous writing allows computation and file writing to overlap, reducing the amount of time spent waiting.

The parameter Q controls the maximum number of pending snapshot tasks.

A small Q limits memory usage but may cause more frequent waiting. A larger Q allows more writes to remain in the background, potentially hiding more I/O latency, but requires more memory for copied grids.

In the tested configurations, Q = 2 represented a practical compromise between execution time and memory usage.

**Conclusion:** Asynchronous snapshot writing substantially reduces I/O waiting time. A bounded number of pending tasks helps balance performance and memory consumption.

---

## 9. Overall Findings

The five experiments demonstrate that parallel and asynchronous execution can improve simulation performance, but the benefits depend on workload size, hardware, and implementation overhead.

The main findings are:

- Strong scaling shows that four threads provide a good balance on the tested processor. Additional threads provide diminishing returns.
- Weak scaling shows that increasing the workload and thread count causes efficiency to decrease because of memory-system limitations and parallel overhead.
- D3 demonstrates that chunk size, partitioning strategy, and thread reuse affect execution time.
- D4 shows that running independent simulations concurrently can be faster than assigning multiple threads to each simulation.
- D5 demonstrates that asynchronous snapshot writing can hide a significant portion of I/O latency by overlapping computation and file writing.

Overall, efficient parallel programming requires careful consideration of processor architecture, memory bandwidth, task granularity, synchronization, and I/O operations.

---

## 10. Limitations

The experiments were performed on a machine with four physical cores and eight logical CPUs. Therefore, the results are specific to the available hardware and execution environment.

Performance can also vary due to operating-system scheduling, background processes, memory bandwidth, cache behaviour, and resource contention.

Asynchronous snapshot writing requires additional memory for copied grids, while excessive task creation can increase scheduling overhead.

The best thread count, chunk size, and partitioning strategy may differ on other hardware or for different simulation workloads.

---

## 11. Conclusion

Part D evaluates the performance of the pandemic simulation through strong scaling, weak scaling, work-unit tuning, independent-run parallelism, and asynchronous I/O.

The experiments show that increasing the number of threads improves performance up to a point, but parallel overhead and hardware limitations prevent ideal scaling.

The work-unit experiments demonstrate the importance of selecting suitable task sizes and partitioning strategies. The independent-run experiment shows that parallelism across simulations can be more effective for workloads containing multiple independent tasks.

Finally, asynchronous snapshot writing significantly reduces waiting time by allowing file-writing operations to overlap with simulation computation.

Overall, the experiments demonstrate that effective parallel and asynchronous programming requires balancing computation, memory access, thread-management overhead, and I/O operations.