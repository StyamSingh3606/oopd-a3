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
#include <queue>
#include <mutex>
#include <condition_variable>
#include <future>
#include <memory>
#include <chrono>
#include <deque>
#include <filesystem>

using namespace std;


// ============================================================
// THREAD POOL
// ============================================================

class ThreadPool {

private:

    vector<thread> workers;
    queue<function<void()>> tasks;

    mutex queueMutex;
    condition_variable condition;

    bool stopping = false;

public:

    explicit ThreadPool(int threadCount) {

        if (threadCount <= 0) {
            throw runtime_error("Thread pool size must be positive");
        }

        workers.reserve(static_cast<size_t>(threadCount));

        for (int i = 0; i < threadCount; i++) {

            workers.emplace_back([this]() {

                while (true) {

                    function<void()> task;

                    {
                        unique_lock<mutex> lock(queueMutex);

                        condition.wait(lock, [this]() {
                            return stopping || !tasks.empty();
                        });

                        if (stopping && tasks.empty()) {
                            return;
                        }

                        task = move(tasks.front());
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
    future<void> submit(Function&& function) {

        auto packaged = make_shared<packaged_task<void()>>(
            forward<Function>(function)
        );

        future<void> result = packaged->get_future();

        {
            lock_guard<mutex> lock(queueMutex);

            if (stopping) {
                throw runtime_error(
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
            lock_guard<mutex> lock(queueMutex);
            stopping = true;
        }

        condition.notify_all();

        for (thread& worker : workers) {

            if (worker.joinable()) {
                worker.join();
            }
        }
    }
};


// ============================================================
// STATES AND DATA STRUCTURES
// ============================================================

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


// ============================================================
// CONFIGURATION
// ============================================================

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

    // Part B and Part C parameters

    int threads = 1;
    int chunks = 1;

    int snapshotEvery = 0;
    int maxPending = 2;
    int ioLatency = 0;
    string snapshotMode = "blocking";
    string snapshotDir = "results/snapshots";

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


// ============================================================
// DETERMINISTIC RANDOM NUMBER GENERATION
// ============================================================

uint64_t hashValue(uint64_t x) {

    x += 0x9e3779b97f4a7c15ULL;

    x = (x ^ (x >> 30)) *
        0xbf58476d1ce4e5b9ULL;

    x = (x ^ (x >> 27)) *
        0x94d049bb133111ebULL;

    return x ^ (x >> 31);
}


double randomValue(
    uint64_t seed,
    int day,
    uint64_t cell,
    int stream
) {

    uint64_t x = hashValue(seed);

    x ^= hashValue(static_cast<uint64_t>(day));

    x ^= hashValue(cell);

    x ^= hashValue(static_cast<uint64_t>(stream));

    uint64_t h = hashValue(x);

    return static_cast<double>(h >> 11) *
           (1.0 / 9007199254740992.0);
}


// ============================================================
// GRID
// ============================================================

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


// ============================================================
// COUNT STATES
// ============================================================

Counts countStates(const Grid& grid) {

    Counts result;

    for (const Person& p : grid.cells) {

        switch (p.state) {

            case S:
                result.susceptible++;
                break;

            case I:
                result.infected++;
                break;

            case R:
                result.recovered++;
                break;

            case V:
                result.vaccinated++;
                break;

            case D:
                result.dead++;
                break;
        }
    }

    return result;
}


// ============================================================
// INITIALIZE GRID
// ============================================================

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
         i < vaccinatedCount + cfg.initialInfected;
         i++) {

        grid.cells[ids[i]].state = I;

        grid.cells[ids[i]].infectedDays = 0;
    }
}


// ============================================================
// COUNT INFECTED NEIGHBOURS
// ============================================================

int infectedNeighbours(
    const Grid& grid,
    int r,
    int c
) {

    int count = 0;

    for (int dr = -1; dr <= 1; dr++) {

        for (int dc = -1; dc <= 1; dc++) {

            if (dr == 0 && dc == 0) {
                continue;
            }

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


// ============================================================
// CHECKSUM
// ============================================================

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


// ============================================================
// SIMULATE ROWS
// ============================================================

void simulateRows(
    const Grid& current,
    Grid& next,
    const Config& cfg,
    int day,
    double betaEff,
    int startRow,
    int endRow
) {

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

                    if (random < cfg.deathProbability) {
                        p.state = D;
                    }
                    else {
                        p.state = R;
                    }

                    p.infectedDays = 0;
                }
            }

            next.cells[id] = p;
        }
    }
}


// ============================================================
// INTERLEAVED ROWS
// ============================================================

void simulateInterleavedRows(
    const Grid& current,
    Grid& next,
    const Config& cfg,
    int day,
    double betaEff,
    int taskId,
    int totalTasks
) {

    for (int row = taskId;
         row < current.N;
         row += totalTasks) {

        simulateRows(
            current,
            next,
            cfg,
            day,
            betaEff,
            row,
            row + 1
        );
    }
}


// ============================================================
// SQUARE BLOCK PARTITION
// ============================================================

void simulateBlock(
    const Grid& current,
    Grid& next,
    const Config& cfg,
    int day,
    double betaEff,
    int startRow,
    int endRow,
    int startCol,
    int endCol
) {

    for (int r = startRow; r < endRow; r++) {

        for (int c = startCol; c < endCol; c++) {

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

                    if (random < cfg.deathProbability) {
                        p.state = D;
                    }
                    else {
                        p.state = R;
                    }

                    p.infectedDays = 0;
                }
            }

            next.cells[id] = p;
        }
    }
}


// ============================================================
// SIMULATE ONE DAY
// ============================================================

void simulateDay(
    const Grid& current,
    Grid& next,
    const Config& cfg,
    int day,
    double betaEff,
    ThreadPool* pool
) {

    // Sequential execution

    if (cfg.mode == "sequential") {

        simulateRows(
            current,
            next,
            cfg,
            day,
            betaEff,
            0,
            current.N
        );

        return;
    }

    const int C = cfg.chunks;

    vector<function<void()>> tasks;

    tasks.reserve(static_cast<size_t>(C));


    // --------------------------------------------------------
    // STRATEGY 1: ROW STRIPS
    // --------------------------------------------------------

    if (cfg.partition == "row") {

        for (int i = 0; i < C; i++) {

            int startRow =
                (current.N * i) / C;

            int endRow =
                (current.N * (i + 1)) / C;

            tasks.emplace_back(
                [&, startRow, endRow]() {

                    simulateRows(
                        current,
                        next,
                        cfg,
                        day,
                        betaEff,
                        startRow,
                        endRow
                    );
                }
            );
        }
    }


    // --------------------------------------------------------
    // STRATEGY 2: SQUARE BLOCKS
    // --------------------------------------------------------

    else if (cfg.partition == "block") {

        int blocksPerSide =
            static_cast<int>(
                ceil(sqrt(static_cast<double>(C)))
            );

        int blockCount =
            blocksPerSide * blocksPerSide;

        int blockSize =
            (current.N + blocksPerSide - 1) /
            blocksPerSide;

        vector<vector<int>> assignedBlocks(
            static_cast<size_t>(C)
        );

        // Distribute blocks among C tasks

        for (int block = 0; block < blockCount; block++) {

            assignedBlocks[block % C].push_back(block);
        }

        for (int taskId = 0; taskId < C; taskId++) {

            vector<int> myBlocks =
                move(assignedBlocks[taskId]);

            tasks.emplace_back(
                [&, myBlocks, blocksPerSide, blockSize]() {

                    for (int block : myBlocks) {

                        int blockRow =
                            block / blocksPerSide;

                        int blockCol =
                            block % blocksPerSide;

                        int startRow =
                            blockRow * blockSize;

                        int endRow =
                            min(
                                startRow + blockSize,
                                current.N
                            );

                        int startCol =
                            blockCol * blockSize;

                        int endCol =
                            min(
                                startCol + blockSize,
                                current.N
                            );

                        if (startRow < endRow &&
                            startCol < endCol) {

                            simulateBlock(
                                current,
                                next,
                                cfg,
                                day,
                                betaEff,
                                startRow,
                                endRow,
                                startCol,
                                endCol
                            );
                        }
                    }
                }
            );
        }
    }


    // --------------------------------------------------------
    // STRATEGY 3: INTERLEAVED ROWS
    // --------------------------------------------------------

    else if (cfg.partition == "interleaved") {

        for (int i = 0; i < C; i++) {

            tasks.emplace_back(
                [&, i, C]() {

                    simulateInterleavedRows(
                        current,
                        next,
                        cfg,
                        day,
                        betaEff,
                        i,
                        C
                    );
                }
            );
        }
    }

    else {

        throw runtime_error(
            "Unknown partition strategy: " + cfg.partition
        );
    }


    // --------------------------------------------------------
    // THREAD-PER-DAY
    // --------------------------------------------------------

    if (cfg.mode == "thread-per-day") {

        vector<thread> dayThreads;

        dayThreads.reserve(tasks.size());

        // Create C new threads for this day

        for (function<void()>& task : tasks) {

            dayThreads.emplace_back(move(task));
        }

        // Join all C threads

        for (thread& worker : dayThreads) {

            worker.join();
        }
    }


    // --------------------------------------------------------
    // THREAD POOL
    // --------------------------------------------------------

    else if (cfg.mode == "thread-pool") {

        if (pool == nullptr) {

            throw runtime_error(
                "Thread pool was not initialized"
            );
        }

        vector<future<void>> completions;

        completions.reserve(tasks.size());

        // Submit C tasks to T persistent workers

        for (function<void()>& task : tasks) {

            completions.push_back(
                pool->submit(move(task))
            );
        }

        // Wait for all tasks to finish

        for (future<void>& completion : completions) {

            completion.get();
        }
    }

    else {

        throw runtime_error(
            "Invalid execution mode: " + cfg.mode
        );
    }
}

// ============================================================
// PART C1: PPM SNAPSHOT WRITER
// ============================================================

void writePPM(
    const Grid& snapshot,
    int day,
    const string& directory,
    int latencyMs
) {
    // Simulate slow storage
    if (latencyMs > 0) {
        this_thread::sleep_for(
            chrono::milliseconds(latencyMs)
        );
    }

    string filename =
        directory + "/snapshot_day_" +
        to_string(day) + ".ppm";

    ofstream image(filename);

    if (!image.is_open()) {
        throw runtime_error(
            "Cannot open snapshot file: " + filename
        );
    }

    // PPM P3 format: width, height, maximum colour value
    image << "P3\n";
    image << snapshot.N << " " << snapshot.N << "\n";
    image << "255\n";

    for (int r = 0; r < snapshot.N; r++) {
        for (int c = 0; c < snapshot.N; c++) {

            const Person& person = snapshot.at(r, c);

            int red = 255;
            int green = 255;
            int blue = 255;

            switch (person.state) {
                case S: // Susceptible: white
                    red = 255; green = 255; blue = 255;
                    break;

                case I: // Infected: red
                    red = 255; green = 0; blue = 0;
                    break;

                case R: // Recovered: green
                    red = 0; green = 255; blue = 0;
                    break;

                case V: // Vaccinated: blue
                    red = 0; green = 0; blue = 255;
                    break;

                case D: // Dead: black
                    red = 0; green = 0; blue = 0;
                    break;
            }

            image << red << " "
                  << green << " "
                  << blue << " ";
        }

        image << "\n";
    }

    image.close();

    if (!image) {
        throw runtime_error(
            "Failed while writing snapshot: " + filename
        );
    }

    cout << "Snapshot saved: " << filename << "\n";
}


// ============================================================
// WRITE RESULTS TO CSV
// ============================================================

void writeResults(
    const Grid& grid,
    int day,
    double betaEff,
    ofstream& file
) {

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


// ============================================================
// PARSE COMMAND LINE ARGUMENTS
// ============================================================

void parseArguments(
    int argc,
    char* argv[],
    Config& cfg
) {

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
                << "--mode <sequential|thread-per-day|thread-pool>\n"
                << "--partition <row|block|interleaved>\n"
                << "--snapshot-every <interval>\n"
                << "--max-pending <maximum-pending-writes>\n"
                << "--io-latency <milliseconds>\n"
                << "--snapshot-mode <blocking|async>\n"
                << "--output <csv-file>\n";

            exit(0);
        }

        if (i + 1 >= argc) {

            throw runtime_error(
                "Missing value for " + arg
            );
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

        else if (arg == "--snapshot-mode")
            cfg.snapshotMode = value;

        else if (arg == "--output")
            cfg.output = value;

        else {

            throw runtime_error(
                "Unknown argument: " + arg
            );
        }
    }
}


// ============================================================
// VALIDATE CONFIGURATION
// ============================================================

void validateConfig(const Config& cfg) {

    if (cfg.N <= 0 || cfg.days < 0) {

        throw runtime_error(
            "Invalid grid size or days"
        );
    }

    uint64_t population =
        static_cast<uint64_t>(cfg.N) * cfg.N;

    if (cfg.initialInfected < 0 ||
        static_cast<uint64_t>(cfg.initialInfected) > population) {

        throw runtime_error(
            "Invalid initial infected count"
        );
    }

    if (cfg.infectionDuration <= 0) {

        throw runtime_error(
            "Duration must be positive"
        );
    }

    if (cfg.infectionRate < 0 ||
        cfg.infectionRate > 1 ||
        cfg.deathProbability < 0 ||
        cfg.deathProbability > 1 ||
        cfg.vaccination < 0 ||
        cfg.vaccination > 1 ||
        cfg.mask < 0 ||
        cfg.mask > 1 ||
        cfg.lockdownFactor < 0 ||
        cfg.lockdownFactor > 1) {

        throw runtime_error(
            "Invalid probability or factor"
        );
    }

    if (cfg.thetaOn < 0 ||
        cfg.thetaOn > 1 ||
        cfg.thetaOff < 0 ||
        cfg.thetaOff > 1 ||
        cfg.thetaOff > cfg.thetaOn) {

        throw runtime_error(
            "Invalid lockdown thresholds"
        );
    }

    if (cfg.threads <= 0 ||
        cfg.chunks <= 0 ||
        cfg.snapshotEvery < 0 ||
        cfg.maxPending <= 0 ||
        cfg.ioLatency < 0) {

        throw runtime_error(
            "Invalid threads or snapshot settings"
        );
    }

    if (cfg.snapshotMode != "blocking" &&
    cfg.snapshotMode != "async") {

    throw runtime_error("Snapshot mode must be blocking or async");
    }

    if (cfg.vaccination * population +
        cfg.initialInfected > population) {

        throw runtime_error(
            "Vaccination and initial infections exceed population"
        );
    }

    if (cfg.mode != "sequential" &&
        cfg.mode != "thread-per-day" &&
        cfg.mode != "thread-pool") {

        throw runtime_error(
            "Invalid execution mode"
        );
    }

    if (cfg.partition != "row" &&
        cfg.partition != "block" &&
        cfg.partition != "interleaved") {

        throw runtime_error(
            "Invalid partition strategy"
        );
    }
}


// ============================================================
// MAIN
// ============================================================

int main(int argc, char* argv[]) {

    try {

        Config cfg;

        parseArguments(argc, argv, cfg);

        validateConfig(cfg);


        // Create current and next grids

        Grid current(cfg.N);

        Grid next(cfg.N);


        // Initialize population

        initialize(current, cfg);


        // Open CSV output

        ofstream file(cfg.output);

        if (!file.is_open()) {

            throw runtime_error(
                "Cannot open output CSV: " + cfg.output
            );
        }

        file << "day,S,I,R,V,D,beta_eff,checksum\n";

        file << setprecision(12);


        // Create persistent thread pool once

        unique_ptr<ThreadPool> pool;

        if (cfg.mode == "thread-pool") {

            pool = make_unique<ThreadPool>(
                cfg.threads
            );
        }

        // Part C1: pending asynchronous snapshot writes
        deque<future<void>> pendingSnapshots;

        int snapshotFailures = 0;

        bool lockdown = false;



        // Record initial state (Day 0)

        double betaEff =
            cfg.infectionRate * cfg.mask;

        Counts counts = countStates(current);

        writeResults(
            current,
            0,
            betaEff,
            file
        );


        double infectedFraction =
            static_cast<double>(counts.infected) /
            current.cells.size();


        if (infectedFraction > cfg.thetaOn) {

            lockdown = true;
        }


        // Simulate subsequent days

        for (int day = 1; day <= cfg.days; day++) {

            betaEff =
                cfg.infectionRate * cfg.mask;

            if (lockdown) {

                betaEff *= cfg.lockdownFactor;
            }


            // Execute selected Part B strategy

            simulateDay(
                current,
                next,
                cfg,
                day,
                betaEff,
                pool.get()
            );


            // Swap only after all tasks finish

            swap(current, next);


            counts = countStates(current);

            writeResults(
                current,
                day,
                betaEff,
                file
            );

            // Part C1: Periodic snapshots
if (cfg.snapshotEvery > 0 &&
    day % cfg.snapshotEvery == 0) {

    if (cfg.snapshotMode == "blocking") {

        try {
            writePPM(
                current,
                day,
                cfg.snapshotDir,
                cfg.ioLatency
            );
        }
        catch (const exception& e) {
            snapshotFailures++;
            cerr << "Snapshot failed: "
                 << e.what() << "\n";
        }
    }
    else if (cfg.snapshotMode == "async") {

        // Enforce maximum pending writes
        if (pendingSnapshots.size() >=
            static_cast<size_t>(cfg.maxPending)) {

            try {
                pendingSnapshots.front().get();
            }
            catch (const exception& e) {
                snapshotFailures++;
                cerr << "Snapshot failed: "
                     << e.what() << "\n";
            }

            pendingSnapshots.pop_front();
        }

        // Copy current grid before submitting background task
        Grid snapshot = current;

        int snapshotDay = day;
        string directory = cfg.snapshotDir;
        int latency = cfg.ioLatency;

        pendingSnapshots.push_back(
            async(
                launch::async,
                [snapshot = move(snapshot),
                 snapshotDay,
                 directory,
                 latency]() {

                    writePPM(
                        snapshot,
                        snapshotDay,
                        directory,
                        latency
                    );
                }
            )
        );
    }
}


            infectedFraction =
                static_cast<double>(counts.infected) /
                current.cells.size();


            // Lockdown decision applies from next day

            if (!lockdown &&
                infectedFraction > cfg.thetaOn) {

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


        while (!pendingSnapshots.empty()) {

    try {
        pendingSnapshots.front().get();
    }
    catch (const exception& e) {
        snapshotFailures++;

        cerr << "Snapshot failed: "
             << e.what() << "\n";
    }

    pendingSnapshots.pop_front();
}

file.close();

cout << "\nSimulation completed!\n";

if (cfg.snapshotEvery > 0) {
    cout << "Snapshot failures: "
         << snapshotFailures << "\n";
}

cout << "Results saved in "
     << cfg.output << "\n";

    }

    catch (const exception& e) {

        cerr << "Error: " << e.what() << "\n";

        return 1;
    }

    return 0;
}