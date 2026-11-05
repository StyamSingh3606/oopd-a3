#!/bin/bash
# Satyam Singh
# MT26140

set -e

mkdir -p results/benchmarks

RAW="results/benchmarks/d5_extended_raw.csv"

echo "mode,latency_ms,K,Q,repeat,elapsed_seconds,max_rss_kb" > "$RAW"

run_test() {
    MODE=$1
    LATENCY=$2
    K=$3
    Q=$4
    REP=$5

    OUTPUT="results/benchmarks/d5x_${MODE}_L${LATENCY}_K${K}_Q${Q}_rep${REP}.csv"
    MEMFILE=$(mktemp)

    echo "Mode=$MODE Latency=$LATENCY K=$K Q=$Q Repeat=$REP"

    START=$(date +%s.%N)

    if [ "$MODE" = "none" ]; then
        /usr/bin/time -f "%M" -o "$MEMFILE" \
            ./build-release/pandemic \
            --N 1000 --days 100 --seed 42 \
            --threads 4 --chunks 16 \
            --mode thread-pool --partition block \
            --snapshot-every 0 \
            --output "$OUTPUT" > /dev/null
    else
        /usr/bin/time -f "%M" -o "$MEMFILE" \
            ./build-release/pandemic \
            --N 1000 --days 100 --seed 42 \
            --threads 4 --chunks 16 \
            --mode thread-pool --partition block \
            --snapshot-every "$K" \
            --max-pending "$Q" \
            --io-latency "$LATENCY" \
            --snapshot-mode "$MODE" \
            --output "$OUTPUT" > /dev/null
    fi

    END=$(date +%s.%N)

    ELAPSED=$(awk -v s="$START" -v e="$END" \
        'BEGIN {printf "%.6f", e-s}')

    RSS=$(cat "$MEMFILE")
    rm -f "$MEMFILE"

    echo "$MODE,$LATENCY,$K,$Q,$REP,$ELAPSED,$RSS" >> "$RAW"

    echo "Elapsed=${ELAPSED}s Peak_RSS=${RSS}KB"
}

# Baseline: no snapshots
for REP in 1 2 3 4 5; do
    run_test none 0 0 0 "$REP"
done

# Blocking and async: vary latency and snapshot interval K
for LATENCY in 50 100 200; do
    for K in 5 10 20; do

        for REP in 1 2 3 4 5; do
            run_test blocking "$LATENCY" "$K" 1 "$REP"
        done

        for Q in 1 2 4; do
            for REP in 1 2 3 4 5; do
                run_test async "$LATENCY" "$K" "$Q" "$REP"
            done
        done

    done
done

echo "D5 EXTENDED EXPERIMENTS COMPLETED"
echo "Results: $RAW"
