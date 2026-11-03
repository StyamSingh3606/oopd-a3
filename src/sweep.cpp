
#include "simulation.h"

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <queue>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

using namespace std;
using Clock = chrono::steady_clock;

struct SweepRun {
    size_t id;
    double beta;
    double vaccination;
    uint64_t seed;
};

struct RunSummary {
    double peakInfectedFraction = 0.0;
    int peakDay = 0;
    uint64_t totalInfected = 0;
    uint64_t totalDeaths = 0;
    int daysSimulated = 0;
};

struct Completion {
    SweepRun run;
    RunSummary summary;
    string error;
};

struct SweepOptions {
    Config base;
    vector<double> betas;
    vector<double> vaccinations;
    vector<uint64_t> seeds;
    int workers = 4;
    int timeLimitSeconds = 60;
    string output = "results/sweep.csv";
};

static string getValue(int& i, int argc, char* argv[]) {
    if (i + 1 >= argc) {
        throw runtime_error(
            "Missing value for argument: " + string(argv[i])
        );
    }
    return argv[++i];
}

template <typename T>
static vector<T> parseList(const string& input) {
    vector<T> values;
    stringstream ss(input);
    string item;

    while (getline(ss, item, ',')) {
        if (item.empty()) {
            throw runtime_error("Empty value in parameter list");
        }

        stringstream parser(item);
        T value;
        char extra;

        if (!(parser >> value) || (parser >> extra)) {
            throw runtime_error("Invalid list value: " + item);
        }

        values.push_back(value);
    }

    if (values.empty()) {
        throw runtime_error("Parameter list cannot be empty");
    }

    return values;
}

static SweepOptions parseSweepArguments(
    int argc,
    char* argv[]
) {
    SweepOptions options;

    for (int i = 2; i < argc; ++i) {
        string arg = argv[i];

        if (arg == "--betas") {
            options.betas =
                parseList<double>(getValue(i, argc, argv));
        }
        else if (arg == "--vaccinations") {
            options.vaccinations =
                parseList<double>(getValue(i, argc, argv));
        }
        else if (arg == "--seeds") {
            options.seeds =
                parseList<uint64_t>(getValue(i, argc, argv));
        }
        else if (arg == "--workers") {
            options.workers =
                stoi(getValue(i, argc, argv));
        }
        else if (arg == "--time-limit") {
            options.timeLimitSeconds =
                stoi(getValue(i, argc, argv));
        }
        else if (arg == "--sweep-output") {
            options.output = getValue(i, argc, argv);
        }
        else if (arg == "--N") {
            options.base.N = stoi(getValue(i, argc, argv));
        }
        else if (arg == "--days") {
            options.base.days = stoi(getValue(i, argc, argv));
        }
        else if (arg == "--I0") {
            options.base.initialInfected =
                stoi(getValue(i, argc, argv));
        }
        else if (arg == "--duration") {
            options.base.infectionDuration =
                stoi(getValue(i, argc, argv));
        }
        else if (arg == "--mu") {
            options.base.deathProbability =
                stod(getValue(i, argc, argv));
        }
        else if (arg == "--mask") {
            options.base.mask = stod(getValue(i, argc, argv));
        }
        else if (arg == "--lambda") {
            options.base.lockdownFactor =
                stod(getValue(i, argc, argv));
        }
        else if (arg == "--theta-on") {
            options.base.thetaOn =
                stod(getValue(i, argc, argv));
        }
        else if (arg == "--theta-off") {
            options.base.thetaOff =
                stod(getValue(i, argc, argv));
        }
        else {
            throw runtime_error("Unknown sweep argument: " + arg);
        }
    }

    if (options.betas.empty() ||
        options.vaccinations.empty() ||
        options.seeds.empty()) {
        throw runtime_error(
            "Please provide --betas, --vaccinations and --seeds"
        );
    }

    if (options.workers <= 0 ||
        options.timeLimitSeconds <= 0) {
        throw runtime_error(
            "Workers and time limit must be positive"
        );
    }

    if (options.base.N <= 0 ||
        options.base.days < 0 ||
        options.base.initialInfected < 0 ||
        options.base.infectionDuration <= 0) {
        throw runtime_error("Invalid simulation dimensions or settings");
    }

    if (options.base.deathProbability < 0 ||
        options.base.deathProbability > 1 ||
        options.base.mask < 0 ||
        options.base.mask > 1 ||
        options.base.lockdownFactor < 0 ||
        options.base.lockdownFactor > 1 ||
        options.base.thetaOn < 0 ||
        options.base.thetaOn > 1 ||
        options.base.thetaOff < 0 ||
        options.base.thetaOff > options.base.thetaOn) {
        throw runtime_error("Invalid simulation probability or threshold");
    }

    for (double beta : options.betas) {
        if (beta < 0 || beta > 1) {
            throw runtime_error("Beta must be between 0 and 1");
        }
    }

    for (double vaccination : options.vaccinations) {
        if (vaccination < 0 || vaccination > 1) {
            throw runtime_error(
                "Vaccination must be between 0 and 1"
            );
        }

        double population =
            static_cast<double>(options.base.N) *
            options.base.N;

        if (vaccination * population +
            options.base.initialInfected > population) {
            throw runtime_error(
                "Vaccination and initial infections exceed population"
            );
        }
    }

    return options;
}

static RunSummary runOneSimulation(
    const Config& cfg
) {
    Grid current(cfg.N);
    Grid next(cfg.N);

    initialize(current, cfg);

    Counts counts = countStates(current);

    const double population =
        static_cast<double>(current.cells.size());

    RunSummary summary;

    summary.peakInfectedFraction =
        static_cast<double>(counts.infected) / population;

    summary.peakDay = 0;

    bool lockdown =
        summary.peakInfectedFraction > cfg.thetaOn;

    int actualDays = 0;

    for (int day = 1; day <= cfg.days; ++day) {
        double betaEff = cfg.infectionRate * cfg.mask;

        if (lockdown) {
            betaEff *= cfg.lockdownFactor;
        }

        // Each sweep worker runs its simulation sequentially.
        simulateDay(
            current,
            next,
            cfg,
            day,
            betaEff,
            nullptr
        );

        swap(current, next);

        counts = countStates(current);
        actualDays = day;

        double infectedFraction =
            static_cast<double>(counts.infected) / population;

        if (infectedFraction > summary.peakInfectedFraction) {
            summary.peakInfectedFraction = infectedFraction;
            summary.peakDay = day;
        }

        // Lockdown decision applies from the next day.
        if (!lockdown && infectedFraction > cfg.thetaOn) {
            lockdown = true;
        }
        else if (lockdown && infectedFraction < cfg.thetaOff) {
            lockdown = false;
        }

        if (counts.infected == 0) {
            break;
        }
    }

    summary.totalInfected =
        counts.infected + counts.recovered + counts.dead;

    summary.totalDeaths = counts.dead;
    summary.daysSimulated = actualDays;

    return summary;
}

int runParameterSweep(int argc, char* argv[]) {
    try {
        SweepOptions options =
            parseSweepArguments(argc, argv);

        vector<SweepRun> jobs;

        size_t id = 0;

        for (double beta : options.betas) {
            for (double vaccination : options.vaccinations) {
                for (uint64_t seed : options.seeds) {
                    jobs.push_back({
                        id++,
                        beta,
                        vaccination,
                        seed
                    });
                }
            }
        }

        const size_t totalRuns = jobs.size();

        ofstream output(options.output);

        if (!output.is_open()) {
            throw runtime_error(
                "Cannot open sweep output: " + options.output
            );
        }

        output
            << "run_id,beta,vaccination,seed,"
            << "peak_infected_fraction,peak_day,"
            << "total_infected,total_deaths,days_simulated,status\n";

        output << setprecision(12);

        cout << "Total parameter combinations: "
             << totalRuns << "\n";

        cout << "Worker threads: "
             << options.workers << "\n";

        cout << "Time limit: "
             << options.timeLimitSeconds << " seconds\n\n";

        mutex mutexJobs;
        condition_variable completionCV;

        queue<Completion> completedQueue;

        size_t nextJob = 0;
        size_t completedCount = 0;
        int exitedWorkers = 0;

        bool stopDispatching = false;

        const Clock::time_point deadline =
            Clock::now() +
            chrono::seconds(options.timeLimitSeconds);

        auto workerFunction = [&]() {
            while (true) {
                size_t jobIndex;

                {
                    lock_guard<mutex> lock(mutexJobs);

                    if (stopDispatching ||
                        Clock::now() >= deadline ||
                        nextJob >= totalRuns) {
                        break;
                    }

                    jobIndex = nextJob++;
                }

                Completion completion;
                completion.run = jobs[jobIndex];

                try {
                    Config cfg = options.base;

                    cfg.infectionRate = completion.run.beta;
                    cfg.vaccination = completion.run.vaccination;
                    cfg.seed = completion.run.seed;

                    // Outer workers provide the parallelism.
                    // Avoid nested thread pools.
                    cfg.mode = "sequential";
                    cfg.snapshotEvery = 0;

                    completion.summary =
                        runOneSimulation(cfg);
                }
                catch (const exception& e) {
                    completion.error = e.what();
                }
                catch (...) {
                    completion.error = "Unknown simulation error";
                }

                {
                    lock_guard<mutex> lock(mutexJobs);
                    completedQueue.push(move(completion));
                }

                completionCV.notify_one();
            }

            {
                lock_guard<mutex> lock(mutexJobs);
                exitedWorkers++;
            }

            completionCV.notify_one();
        };

        vector<thread> workers;

        int workerCount = min(
            options.workers,
            static_cast<int>(totalRuns)
        );

        for (int i = 0; i < workerCount; ++i) {
            workers.emplace_back(workerFunction);
        }

        // Coordinator: writes results in completion order.
        while (true) {
            Completion completion;
            bool hasCompletion = false;
            bool allWorkersExited = false;

            {
                unique_lock<mutex> lock(mutexJobs);

                completionCV.wait(
                    lock,
                    [&]() {
                        return !completedQueue.empty() ||
                               exitedWorkers == workerCount;
                    }
                );

                if (!completedQueue.empty()) {
                    completion = move(completedQueue.front());
                    completedQueue.pop();
                    hasCompletion = true;
                }

                allWorkersExited =
                    exitedWorkers == workerCount &&
                    completedQueue.empty();
            }

            if (hasCompletion) {
                const SweepRun& run = completion.run;
                const RunSummary& s = completion.summary;

                if (completion.error.empty()) {
                    output
                        << run.id << ","
                        << run.beta << ","
                        << run.vaccination << ","
                        << run.seed << ","
                        << s.peakInfectedFraction << ","
                        << s.peakDay << ","
                        << s.totalInfected << ","
                        << s.totalDeaths << ","
                        << s.daysSimulated
                        << ",success\n";
                }
                else {
                    output
                        << run.id << ","
                        << run.beta << ","
                        << run.vaccination << ","
                        << run.seed
                        << ",,,,,,"
                        << "failed\n";

                    cerr << "Run " << run.id
                         << " failed: "
                         << completion.error << "\n";
                }

                output.flush();

                completedCount++;

                cout << "\rProgress: "
                     << completedCount << "/"
                     << totalRuns << flush;
            }

            if (allWorkersExited) {
                break;
            }
        }

        for (thread& worker : workers) {
            worker.join();
        }

        output.close();

        const size_t missingRuns =
            totalRuns - completedCount;

        cout << "\n\nParameter sweep completed.\n";

        cout << "Completed runs: "
             << completedCount << "/" << totalRuns << "\n";

        cout << "Missing runs: "
             << missingRuns << "\n";

        if (missingRuns > 0) {
            cout << "Time limit reached or some runs were not started.\n";
        }

        cout << "Results saved in: "
             << options.output << "\n";

        return 0;
    }
    catch (const exception& e) {
        cerr << "Sweep error: " << e.what() << "\n";
        return 1;
    }
}
