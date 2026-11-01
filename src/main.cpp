#include <iostream>
#include <vector>
#include <random>
#include <fstream>
#include <cstdint>
#include <algorithm>
#include <cmath>
#include <string>
#include <stdexcept>
#include <iomanip>
#include <thread>
#include <functional>

using namespace std;

// Five possible states
enum State { S, I, R, V, D };

struct Person {
    State state = S;
    int infectedDays = 0;
};

// Simulation configuration
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

    // Reserved for Parts B and C
    int threads = 1;
    int chunks = 1;
    int snapshotEvery = 0;
    int maxPending = 2;
    int ioLatency = 0;

    string mode = "sequential";
    string partition = "row";

    string output = "results/sequential.csv";
};

struct Counts {
    uint64_t susceptible = 0;
    uint64_t infected = 0;
    uint64_t recovered = 0;
    uint64_t vaccinated = 0;
    uint64_t dead = 0;
};

// SplitMix64 deterministic hash
uint64_t hashValue(uint64_t x) {
    x += 0x9e3779b97f4a7c15ULL;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
    return x ^ (x >> 31);
}

// Deterministic random value for a particular cell and day
double randomValue(uint64_t seed, int day,
                   uint64_t cell, int stream) {

    uint64_t x = hashValue(seed);
    x ^= hashValue(static_cast<uint64_t>(day));
    x ^= hashValue(cell);
    x ^= hashValue(static_cast<uint64_t>(stream));

    uint64_t h = hashValue(x);

    return static_cast<double>(h >> 11) *
           (1.0 / 9007199254740992.0);
}

// Grid representation
struct Grid {
    int N;
    vector<Person> cells;

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

// Count people in each state
Counts countStates(const Grid& grid) {
    Counts result;

    for (const Person& p : grid.cells) {
        switch (p.state) {
            case S: result.susceptible++; break;
            case I: result.infected++; break;
            case R: result.recovered++; break;
            case V: result.vaccinated++; break;
            case D: result.dead++; break;
        }
    }

    return result;
}

// Initialize the grid
void initialize(Grid& grid, const Config& cfg) {

    vector<size_t> ids(grid.cells.size());

    for (size_t i = 0; i < ids.size(); i++) {
        ids[i] = i;
    }

    mt19937_64 rng(cfg.seed);
    shuffle(ids.begin(), ids.end(), rng);

    size_t total = ids.size();

    size_t vaccinatedCount = static_cast<size_t>(
        round(cfg.vaccination * total)
    );

    if (vaccinatedCount + cfg.initialInfected > total) {
        throw runtime_error(
            "Vaccination and initial infections exceed population"
        );
    }

    // Vaccinate randomly selected people
    for (size_t i = 0; i < vaccinatedCount; i++) {
        grid.cells[ids[i]].state = V;
    }

    // Infect randomly selected susceptible people
    for (size_t i = vaccinatedCount;
         i < vaccinatedCount + cfg.initialInfected; i++) {

        grid.cells[ids[i]].state = I;
        grid.cells[ids[i]].infectedDays = 0;
    }
}

// Count infected neighbours in eight directions
int infectedNeighbours(const Grid& grid, int r, int c) {

    int count = 0;

    for (int dr = -1; dr <= 1; dr++) {
        for (int dc = -1; dc <= 1; dc++) {

            if (dr == 0 && dc == 0)
                continue;

            int nr = r + dr;
            int nc = c + dc;

            // No wrapping at grid boundaries
            if (nr >= 0 && nr < grid.N &&
                nc >= 0 && nc < grid.N) {

                if (grid.at(nr, nc).state == I) {
                    count++;
                }
            }
        }
    }

    return count;
}

// Calculate a checksum of the complete grid
uint64_t calculateChecksum(const Grid& grid) {

    uint64_t checksum = 0;

    for (size_t i = 0; i < grid.cells.size(); i++) {

        const Person& p = grid.cells[i];

        uint64_t value =
            static_cast<uint64_t>(p.state) * 1000003ULL +
            static_cast<uint64_t>(p.infectedDays);

        checksum ^= hashValue(
            static_cast<uint64_t>(i) ^ hashValue(value)
        );
    }

    return checksum;
}

// Calculate the next day using two separate grids
void simulateRows(const Grid& current,
                  Grid& next,
                  const Config& cfg,
                  int day,
                  double betaEff,
                  int startRow,
                  int endRow) {

    for (int r = startRow; r < endRow; r++) {

        for (int c = 0; c < current.N; c++) {

            size_t id =
                static_cast<size_t>(r) * current.N + c;

            Person p = current.cells[id];

            if (p.state == S) {

                int k = infectedNeighbours(current, r, c);

                double probability =
                    1.0 - pow(1.0 - betaEff, k);

                double random =
                    randomValue(cfg.seed, day, id, 0);

                if (random < probability) {
                    p.state = I;
                    p.infectedDays = 0;
                }
            }

            else if (p.state == I) {

                p.infectedDays++;

                if (p.infectedDays >= cfg.infectionDuration) {

                    double random =
                        randomValue(cfg.seed, day, id, 1);

                    if (random < cfg.deathProbability)
                        p.state = D;
                    else
                        p.state = R;

                    p.infectedDays = 0;
                }
            }

            next.cells[id] = p;
        }
    }
}

// Sequential or thread-per-day execution
void simulateDay(const Grid& current, Grid& next, const Config& cfg, int day, double betaEff) {

    if (cfg.mode == "sequential") {

        simulateRows(current, next, cfg, day, betaEff, 0, current.N);

        return;
    }

    if (cfg.mode == "thread-per-day") {

        int T = min(cfg.threads, current.N);

        vector<thread> workers;
        workers.reserve(T);

        int rowsPerThread = current.N / T;
        int extraRows = current.N % T;

        int startRow = 0;

        for (int i = 0; i < T; i++) {

            int rows = rowsPerThread +
                       (i < extraRows ? 1 : 0);

            int endRow = startRow + rows;

            workers.emplace_back(
                simulateRows,
                cref(current),
                ref(next),
                cref(cfg),
                day,
                betaEff,
                startRow,
                endRow
            );

            startRow = endRow;
        }

        // Wait for every worker before swapping grids
        for (thread& worker : workers) {
            worker.join();
        }

        return;
    }

    throw runtime_error("Unknown execution mode: " + cfg.mode);
}





// Write daily statistics to CSV
void writeResults(const Grid& grid,
                  int day,
                  double betaEff,
                  ofstream& file) {

    Counts c = countStates(grid);

    uint64_t checksum = calculateChecksum(grid);

    file << day << ","
         << c.susceptible << ","
         << c.infected << ","
         << c.recovered << ","
         << c.vaccinated << ","
         << c.dead << ","
         << betaEff << ","
         << checksum << "\n";

    cout << "Day " << day
         << " | S: " << c.susceptible
         << " I: " << c.infected
         << " R: " << c.recovered
         << " V: " << c.vaccinated
         << " D: " << c.dead
         << " | beta_eff: " << betaEff
         << "\n";
}

// Parse command-line arguments
void parseArguments(int argc, char* argv[], Config& cfg) {

    for (int i = 1; i < argc; i++) {

        string arg = argv[i];

        if (arg == "--help") {
            cout
                << "Pandemic simulation options:\n"
                << "--N <grid-size>\n"
                << "--days <number-of-days>\n"
                << "--seed <random-seed>\n"
                << "--I0 <initial-infected>\n"
                << "--duration <infection-duration>\n"
                << "--beta <infection-rate>\n"
                << "--mu <death-probability>\n"
                << "--v <vaccination-fraction>\n"
                << "--mask <mask-factor>\n"
                << "--lambda <lockdown-factor>\n"
                << "--theta-on <lockdown-start-threshold>\n"
                << "--theta-off <lockdown-end-threshold>\n"
                << "--threads <thread-count>\n"
                << "--chunks <chunk-count>\n"
                << "--snapshot-every <interval>\n"
                << "--max-pending <maximum-pending-writes>\n"
                << "--io-latency <milliseconds>\n"
                << "--output <csv-file>\n";
            exit(0);
        }

        if (i + 1 >= argc) {
            throw runtime_error("Missing value for " + arg);
}

string value = argv[++i];

if (arg == "--N")
    cfg.N = stoi(value);

else if (arg == "--days")
    cfg.days = stoi(value);

else if (arg == "--seed")
    cfg.seed = stoull(value);

else if (arg == "--I0")
    cfg.initialInfected = stoi(value);

else if (arg == "--duration")
    cfg.infectionDuration = stoi(value);

else if (arg == "--beta")
    cfg.infectionRate = stod(value);

else if (arg == "--mu")
    cfg.deathProbability = stod(value);

else if (arg == "--v")
    cfg.vaccination = stod(value);

else if (arg == "--mask")
    cfg.mask = stod(value);

else if (arg == "--lambda")
    cfg.lockdownFactor = stod(value);

else if (arg == "--theta-on")
    cfg.thetaOn = stod(value);

else if (arg == "--theta-off")
    cfg.thetaOff = stod(value);

else if (arg == "--threads")
    cfg.threads = stoi(value);

else if (arg == "--chunks")
    cfg.chunks = stoi(value);

else if (arg == "--mode")
    cfg.mode = value;

else if (arg == "--partition")
    cfg.partition = value;

else if (arg == "--snapshot-every")
    cfg.snapshotEvery = stoi(value);

else if (arg == "--max-pending")
    cfg.maxPending = stoi(value);

else if (arg == "--io-latency")
    cfg.ioLatency = stoi(value);

else if (arg == "--output")
    cfg.output = value;

else
    throw runtime_error("Unknown argument: " + arg);
}
}

// Validate parameters
void validateConfig(const Config& cfg) {

    if (cfg.N <= 0 || cfg.days < 0)
        throw runtime_error("Invalid grid size or days");

    uint64_t population =
        static_cast<uint64_t>(cfg.N) * cfg.N;

    if (cfg.initialInfected < 0 ||
        static_cast<uint64_t>(cfg.initialInfected) > population) {
        throw runtime_error("Invalid initial infected count");
    }

    if (cfg.infectionDuration <= 0)
        throw runtime_error("Duration must be positive");

    if (cfg.infectionRate < 0 || cfg.infectionRate > 1 ||
        cfg.deathProbability < 0 || cfg.deathProbability > 1 ||
        cfg.vaccination < 0 || cfg.vaccination > 1 ||
        cfg.mask < 0 || cfg.mask > 1 ||
        cfg.lockdownFactor < 0 || cfg.lockdownFactor > 1) {
        throw runtime_error("Invalid probability or factor");
    }

    if (cfg.thetaOn < 0 || cfg.thetaOn > 1 ||
        cfg.thetaOff < 0 || cfg.thetaOff > 1 ||
        cfg.thetaOff > cfg.thetaOn) {
        throw runtime_error("Invalid lockdown thresholds");
    }

    if (cfg.threads <= 0 || cfg.chunks <= 0 ||
        cfg.snapshotEvery < 0 || cfg.maxPending <= 0 ||
        cfg.ioLatency < 0) {
        throw runtime_error("Invalid threads or snapshot settings");
    }

    if (cfg.vaccination * population +
        cfg.initialInfected > population) {
        throw runtime_error(
            "Vaccination and initial infections exceed population"
        );
    }
    if (cfg.mode != "sequential" &&
    cfg.mode != "thread-per-day") {
    throw runtime_error("Invalid execution mode");
    }

    if (cfg.partition != "row") {
    throw runtime_error("Only row partition is implemented currently");
    }
}

int main(int argc, char* argv[]) {

    try {

        Config cfg;

        parseArguments(argc, argv, cfg);
        validateConfig(cfg);

        Grid current(cfg.N);
        Grid next(cfg.N);

        initialize(current, cfg);

        ofstream file(cfg.output);

        if (!file.is_open()) {
            throw runtime_error(
                "Cannot open output CSV: " + cfg.output
            );
        }

        file << "day,S,I,R,V,D,beta_eff,checksum\n";
        file << setprecision(12);

        bool lockdown = false;

        // Record initial state (Day 0)
        double betaEff = cfg.infectionRate * cfg.mask;

        Counts counts = countStates(current);

        writeResults(current, 0, betaEff, file);

        double infectedFraction =
            static_cast<double>(counts.infected) /
            current.cells.size();

        if (infectedFraction > cfg.thetaOn) {
            lockdown = true;
        }

        // Simulate subsequent days
        for (int day = 1; day <= cfg.days; day++) {

            betaEff = cfg.infectionRate * cfg.mask;

            if (lockdown) {
                betaEff *= cfg.lockdownFactor;
            }

            simulateDay(current, next, cfg, day, betaEff);

            // Swap after the complete day
            swap(current, next);

            counts = countStates(current);

            writeResults(current, day, betaEff, file);

            infectedFraction =
                static_cast<double>(counts.infected) /
                current.cells.size();

            // Decision applies from the next day
            if (!lockdown && infectedFraction > cfg.thetaOn) {
                lockdown = true;
            }
            else if (lockdown &&
                     infectedFraction < cfg.thetaOff) {
                lockdown = false;
            }

            if (counts.infected == 0) {
                cout << "\nNo infected people remain. "
                     << "Simulation stopped.\n";
                break;
            }
        }

        file.close();

        cout << "\nSimulation completed!\n";
        cout << "Results saved in " << cfg.output << "\n";

    }
    catch (const exception& e) {

        cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
