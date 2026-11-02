#Satyam Singh
#MT26140
#!/usr/bin/env bash
set -euo pipefail

EXE="./build/pandemic"
N=1000
DAYS=100
SEED=42
REPEATS=3
POOL_CHUNKS=16

mkdir -p results/benchmarks

CSV="results/benchmarks/part_b_timings.csv"
TIMEFILE="/tmp/pandemic_time.txt"
OUTPUT="results/benchmarks/benchmark_run.csv"

echo "mode,partition,threads,chunks,run,elapsed_seconds" > "$CSV"

run_benchmark() {
    local MODE=$1
    local PARTITION=$2
    local THREADS=$3
    local CHUNKS=$4
    local RUN=$5

    /usr/bin/time -f "%e" -o "$TIMEFILE" \
        "$EXE" \
        --N "$N" \
        --days "$DAYS" \
        --seed "$SEED" \
        --mode "$MODE" \
        --partition "$PARTITION" \
        --threads "$THREADS" \
        --chunks "$CHUNKS" \
        --output "$OUTPUT" \
        > /dev/null

    ELAPSED=$(cat "$TIMEFILE")

    echo "$MODE,$PARTITION,$THREADS,$CHUNKS,$RUN,$ELAPSED" >> "$CSV"

    echo "Completed: $MODE | $PARTITION | T=$THREADS | Run=$RUN | ${ELAPSED}s"
}

echo "Starting sequential benchmarks..."

for RUN in $(seq 1 "$REPEATS"); do
    run_benchmark sequential row 1 1 "$RUN"
done

THREAD_COUNTS=(1 2 4 8 16)
PARTITIONS=(row block interleaved)
MODES=(thread-per-day thread-pool)

for MODE in "${MODES[@]}"; do
    for PARTITION in "${PARTITIONS[@]}"; do
        for T in "${THREAD_COUNTS[@]}"; do

            if [ "$MODE" = "thread-per-day" ]; then
                CHUNKS=$T
            else
                CHUNKS=$POOL_CHUNKS
            fi

            for RUN in $(seq 1 "$REPEATS"); do
                run_benchmark "$MODE" "$PARTITION" "$T" "$CHUNKS" "$RUN"
            done

        done
    done
done

echo ""
echo "Benchmark completed!"
echo "Raw timings saved in: $CSV"
