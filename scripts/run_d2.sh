
#!/bin/bash

set -euo pipefail

BIN="./build-release/pandemic"
OUT="results/benchmarks/d2_weak_scaling_raw.csv"
TMP="results/benchmarks/d2_temp.csv"

mkdir -p results/benchmarks
rm -f "$OUT" "$TMP"

echo "repetition,N,total_cells,threads,chunks,mode,partition,elapsed_seconds,checksum" > "$OUT"

# Approximately 1 million cells per thread
THREAD_COUNTS=(1 2 4 8)
GRID_SIZES=(1000 1414 2000 2828)

REPETITIONS=5

for i in "${!THREAD_COUNTS[@]}"; do

    T=${THREAD_COUNTS[$i]}
    N=${GRID_SIZES[$i]}
    C=$((4 * T))

    for REP in $(seq 1 "$REPETITIONS"); do

        echo "========================================"
        echo "D2: Threads=$T | Grid=${N}x${N} | Repetition=$REP"
        echo "========================================"

        rm -f "$TMP"

        if ! "$BIN" \
            --benchmark \
            --N "$N" \
            --days 200 \
            --seed 42 \
            --threads "$T" \
            --chunks "$C" \
            --mode thread-pool \
            --partition block \
            --benchmark-output "$TMP" \
            > "results/benchmarks/d2_T${T}_R${REP}.log" 2>&1; then

            cat "results/benchmarks/d2_T${T}_R${REP}.log"
            echo "Experiment failed: T=$T, repetition=$REP"
            exit 1
        fi

        tail -n 1 "$TMP" |
            awk -F',' -v rep="$REP" -v cells="$((N*N))" \
            '{print rep "," $1 "," cells "," $5 "," $6 "," $7 "," $8 "," $9 "," $10}' \
            >> "$OUT"

        grep -E "Elapsed time|Executed days|Checksum" \
            "results/benchmarks/d2_T${T}_R${REP}.log"

    done
done

rm -f "$TMP"

echo
echo "All D2 experiments completed."
echo "Raw results: $OUT"
