#Satyam Singh
#MT26140
#!/bin/bash

set -e

mkdir -p results/benchmarks

RAW="results/benchmarks/d5_raw.csv"

echo "mode,latency_ms,Q,repeat,elapsed_seconds" > "$RAW"

run_test() {
    MODE=$1
    LATENCY=$2
    Q=$3
    REP=$4

    OUTPUT="results/benchmarks/d5_${MODE}_L${LATENCY}_Q${Q}_rep${REP}.csv"

    echo "======================================"
    echo "Mode=$MODE Latency=$LATENCY Q=$Q Repeat=$REP"
    echo "======================================"

    START=$(date +%s.%N)

    if [ "$MODE" = "none" ]; then
        ./build-release/pandemic \
            --N 1000 \
            --days 100 \
            --seed 42 \
            --threads 4 \
            --chunks 16 \
            --mode thread-pool \
            --partition block \
            --snapshot-every 0 \
            --output "$OUTPUT" > /dev/null

    else
        ./build-release/pandemic \
            --N 1000 \
            --days 100 \
            --seed 42 \
            --threads 4 \
            --chunks 16 \
            --mode thread-pool \
            --partition block \
            --snapshot-every 10 \
            --max-pending "$Q" \
            --io-latency "$LATENCY" \
            --snapshot-mode "$MODE" \
            --output "$OUTPUT" > /dev/null
    fi

    END=$(date +%s.%N)

    ELAPSED=$(awk -v s="$START" -v e="$END" \
        'BEGIN {printf "%.6f", e-s}')

    echo "$MODE,$LATENCY,$Q,$REP,$ELAPSED" >> "$RAW"

    echo "Elapsed: $ELAPSED seconds"
}

# Baseline: no snapshots
for REP in 1 2 3 4 5
do
    run_test none 0 0 "$REP"
done

# Blocking and async comparisons
for LATENCY in 50 100 200
do
    for REP in 1 2 3 4 5
    do
        run_test blocking "$LATENCY" 1 "$REP"
    done

    for Q in 1 2 4
    do
        for REP in 1 2 3 4 5
        do
            run_test async "$LATENCY" "$Q" "$REP"
        done
    done
done

echo "======================================"
echo "D5 EXPERIMENTS COMPLETED"
echo "Raw results: $RAW"
