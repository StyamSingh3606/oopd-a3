
#ifndef SIMULATION_H
#define SIMULATION_H

#include <cstdint>
#include <string>
#include <vector>

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

// ThreadPool is defined in main.cpp.
// We only need its forward declaration here.
class ThreadPool;

// ------------------------------------------------------------
// SHARED SIMULATION FUNCTIONS
// ------------------------------------------------------------

Counts countStates(const Grid& grid);

void initialize(Grid& grid, const Config& cfg);

void simulateDay(const Grid& current,Grid& next,const Config& cfg, int day,double betaEff,ThreadPool* pool);

int runParameterSweep(int argc, char* argv[]);

#endif
