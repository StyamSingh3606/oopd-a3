#ifndef SIMULATION_H
#define SIMULATION_H

#include <cstdint>
#include <string>
#include <vector>
#include <thread>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <future>
#include <memory>
#include <vector>
#include <stdexcept>
#include <utility>

// ------------------------------------------------------------
// STATES
// ------------------------------------------------------------

enum State {
    S,
    I,
    R,
    V,
    D
};

struct Person {
    State state = S;
    int infectedDays = 0;
};

// ------------------------------------------------------------
// CONFIGURATION
// ------------------------------------------------------------

struct Config {
    int N = 100;
    int days = 50;

    int initialInfected = 10;
    int infectionDuration = 7;

    double infectionRate = 0.08;
    double deathProbability = 0.01;

    double vaccination = 0.0;
    double mask = 1.0;

    double lockdownFactor = 0.3;

    double thetaOn = 0.02;
    double thetaOff = 0.005;

    uint64_t seed = 42;

    int threads = 1;
    int chunks = 1;

    int snapshotEvery = 0;
    int maxPending = 2;
    int ioLatency = 0;

    std::string snapshotMode = "blocking";
    std::string snapshotDir = "results/snapshots";

    std::string mode = "sequential";
    std::string partition = "row";

    std::string output = "results/sequential.csv";
    bool benchmarkMode = false;
    std::string benchmarkOutput = "results/benchmarks/d1.csv";
};

class ThreadPool {
private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;

    std::mutex queueMutex;
    std::condition_variable condition;

    bool stopping = false;

public:
    explicit ThreadPool(int threadCount) {
        if (threadCount <= 0) {
            throw std::runtime_error("Thread pool size must be positive");
        }

        workers.reserve(static_cast<size_t>(threadCount));

        for (int i = 0; i < threadCount; i++) {
            workers.emplace_back([this]() {
                while (true) {
                    std::function<void()> task;

                    {
                        std::unique_lock<std::mutex> lock(queueMutex);

                        condition.wait(lock, [this]() {
                            return stopping || !tasks.empty();
                        });

                        if (stopping && tasks.empty()) {
                            return;
                        }

                        task = std::move(tasks.front());
                        tasks.pop();
                    }

                    task();
                }
            });
        }
    }

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    template <typename Function>
    std::future<void> submit(Function&& function) {
        auto packaged = std::make_shared<std::packaged_task<void()>>(
            std::forward<Function>(function)
        );

        std::future<void> result = packaged->get_future();

        {
            std::lock_guard<std::mutex> lock(queueMutex);

            if (stopping) {
                throw std::runtime_error(
                    "Cannot submit to a stopped thread pool"
                );
            }

            tasks.emplace([packaged]() {
                (*packaged)();
            });
        }

        condition.notify_one();
        return result;
    }

    ~ThreadPool() {
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            stopping = true;
        }

        condition.notify_all();

        for (std::thread& worker : workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }
};

// ------------------------------------------------------------
// COUNTS
// ------------------------------------------------------------

struct Counts {
    uint64_t susceptible = 0;
    uint64_t infected = 0;
    uint64_t recovered = 0;
    uint64_t vaccinated = 0;
    uint64_t dead = 0;
};

// ------------------------------------------------------------
// GRID
// ------------------------------------------------------------

struct Grid {
    int N;
    std::vector<Person> cells;

    explicit Grid(int size)
        : N(size),
          cells(static_cast<size_t>(size) * size) {}

    Person& at(int r, int c) {
        return cells[static_cast<size_t>(r) * N + c];
    }

    const Person& at(int r, int c) const {
        return cells[static_cast<size_t>(r) * N + c];
    }
};
// ------------------------------------------------------------
// SHARED SIMULATION FUNCTIONS
// ------------------------------------------------------------

Counts countStates(const Grid& grid);

void initialize(Grid& grid, const Config& cfg);

void simulateDay(const Grid& current,Grid& next,const Config& cfg, int day,double betaEff,ThreadPool* pool);

int runParameterSweep(int argc, char* argv[]);

#endif
