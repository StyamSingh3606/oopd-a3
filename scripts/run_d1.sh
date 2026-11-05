#!/bin/bash

set -euo pipefail

BIN="./build-release/pandemic"
OUT="results/benchmarks/d1_strong_scaling_raw.csv"
TMP="results/benchmarks/d1_temp.csv"

mkdir -p results/benchmarks

rm -f "$OUT" "$TMP"

echo "repetition,N,requested_days,executed_days,seed,threads,chunks,mode,partition,elapsed_seconds,checksum" > "$OUT"

THREAD_COUNTS=(1 2 4 8 16)
REPETITIONS=5

for T in "${THREAD_COUNTS[@]}"; do

    C=$((4 * T))

    for REP in $(seq 1 "$REPETITIONS"); do

        echo "========================================"
        echo "Running: Threads=$T | Chunks=$C | Repetition=$REP"
        echo "========================================"

        rm -f "$TMP"

        if ! "$BIN" \
            --benchmark \
            --N 4000 \
            --days 200 \
            --seed 42 \
            --threads "$T" \
            --chunks "$C" \
            --mode thread-pool \
            --partition block \
            --benchmark-output "$TMP" \
            > "results/benchmarks/d1_T${T}_R${REP}.log" 2>&1; then

            cat "results/benchmarks/d1_T${T}_R${REP}.log"
            echo "Experiment failed: T=$T, repetition=$REP"
            exit 1
        fi

        tail -n 1 "$TMP" |
            awk -v rep="$REP" '{print rep "," $0}' >> "$OUT"

        grep -E "Elapsed time|Executed days|Checksum" \
            "results/benchmarks/d1_T${T}_R${REP}.log"

    done
done

rm -f "$TMP"

echo
echo "All D1 experiments completed."
echo "Raw results: $OUT"