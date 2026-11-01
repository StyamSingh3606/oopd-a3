
#include <iostream>
#include <vector>
#include <random>
#include <fstream>
#include <cstdlib>
#include <cstdint>
#include <algorithm>
#include <cmath>
#include <stdexcept>

using namespace std;

// Five population states
enum State {
    S, I, R, V, D
};

// Each person
struct Person {
    State state = S;
    int days = 0;
};

// Simulation parameters
int N = 100;
int totalDays = 50;
int I0 = 10;
int duration = 7;

double infectionRate = 0.08;
double mu = 0.01;
double vaccination = 0.0;
double mask = 1.0;

uint64_t seed = 42;

// Two grids
vector<Person> currentGrid;
vector<Person> nextGrid;

// Deterministic random number
uint64_t hashValue(uint64_t x) {
    x += 0x9e3779b97f4a7c15ULL;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
    return x ^ (x >> 31);
}

double randomValue(int day, int cell, int stream) {
    uint64_t x = seed;
    x ^= hashValue(day);
    x ^= hashValue(cell);
    x ^= hashValue(stream);

    return (hashValue(x) >> 11) *
           (1.0 / 9007199254740992.0);
}

// Convert row and column into index
int index(int row, int col) {
    return row * N + col;
}

// Initialize population
void initialize() {

    int total = N * N;

    vector<int> ids(total);

    for (int i = 0; i < total; i++)
        ids[i] = i;

    mt19937_64 rng(seed);
    shuffle(ids.begin(), ids.end(), rng);

    int vaccinated = round(vaccination * total);

    for (int i = 0; i < vaccinated; i++)
        currentGrid[ids[i]].state = V;

    if (I0 > total - vaccinated)
        throw runtime_error("Too many initially infected people");

    for (int i = 0; i < I0; i++) {
        currentGrid[ids[vaccinated + i]].state = I;
        currentGrid[ids[vaccinated + i]].days = 0;
    }
}

// Count infected neighbours
int infectedNeighbours(int row, int col) {

    int count = 0;

    for (int dr = -1; dr <= 1; dr++) {
        for (int dc = -1; dc <= 1; dc++) {

            if (dr == 0 && dc == 0)
                continue;

            int r = row + dr;
            int c = col + dc;

            if (r < 0 || r >= N || c < 0 || c >= N)
                continue;

            if (currentGrid[index(r, c)].state == I)
                count++;
        }
    }

    return count;
}

// Update one complete day
void simulateDay(int day, double betaEff) {

    for (int row = 0; row < N; row++) {
        for (int col = 0; col < N; col++) {

            int id = index(row, col);

            Person p = currentGrid[id];

            if (p.state == S) {

                int k = infectedNeighbours(row, col);

                double probability =
                    1.0 - pow(1.0 - betaEff, k);

                if (randomValue(day, id, 0) < probability) {
                    p.state = I;
                    p.days = 0;
                }
            }

            else if (p.state == I) {

                p.days++;

                if (p.days == duration) {

                    if (randomValue(day, id, 1) < mu)
                        p.state = D;
                    else
                        p.state = R;

                    p.days = 0;
                }
            }

            nextGrid[id] = p;
        }
    }

    currentGrid.swap(nextGrid);
}

// Count population
void printCounts(int day, ofstream& file, double betaEff) {

    int s = 0, i = 0, r = 0, v = 0, d = 0;

    for (const Person& p : currentGrid) {

        switch (p.state) {
            case S: s++; break;
            case I: i++; break;
            case R: r++; break;
            case V: v++; break;
            case D: d++; break;
        }
    }

    // Simple grid checksum
    uint64_t checksum = 0;

    for (size_t id = 0; id < currentGrid.size(); id++) {
        checksum = hashValue(
            checksum ^ (static_cast<uint64_t>(currentGrid[id].state)
                        + id * 5)
        );
        checksum = hashValue(
            checksum ^ static_cast<uint64_t>(
                currentGrid[id].days + 1
            )
        );
    }

    cout << "Day " << day
         << " | S: " << s
         << " I: " << i
         << " R: " << r
         << " V: " << v
         << " D: " << d << endl;

    file << day << "," << s << "," << i << ","
         << r << "," << v << "," << d << ","
         << betaEff << "," << checksum << "\n";
}

// Main function
int main() {

    currentGrid.resize(N * N);
    nextGrid.resize(N * N);

    initialize();

    ofstream file("results/sequential.csv");

    if (!file.is_open()) {
        cerr << "Cannot open results/sequential.csv\n";
        return 1;
    }

    file << "day,S,I,R,V,D,beta_eff,checksum\n";

    bool lockdown = false;

    for (int day = 0; day <= totalDays; day++) {

        double betaEff = infectionRate * mask;

        if (lockdown)
            betaEff *= 0.3;

        if (day > 0) {
            simulateDay(day, betaEff);
        }

        printCounts(day, file, betaEff);

        // Lockdown decision applies to next day
        int infected = 0;

        for (const Person& p : currentGrid) {
            if (p.state == I)
                infected++;
        }

        double fraction = double(infected) / (N * N);

        if (!lockdown && fraction > 0.02)
            lockdown = true;

        else if (lockdown && fraction < 0.005)
            lockdown = false;

        if (infected == 0)
            break;
    }

    file.close();

    cout << "\nSimulation completed!\n";
    cout << "Results saved in results/sequential.csv\n";

    return 0;
}
