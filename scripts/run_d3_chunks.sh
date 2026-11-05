#!/bin/bash
set -euo pipefail

BIN="./build-release/pandemic"
OUT="results/benchmarks/d3_chunks_raw.csv"
TMP="results/benchmarks/d3_temp.csv"

mkdir -p results/benchmarks
rm -f "$OUT" "$TMP"

echo "repetition,N,threads,chunks,mode,partition,elapsed_seconds,checksum" > "$OUT"

THREADS=4
N=4000
REPETITIONS=5
CHUNKS=(4 8 16 32 64 128 256)

for C in "${CHUNKS[@]}"; do
    for REP in $(seq 1 "$REPETITIONS"); do
        echo "D3.1: Chunks=$C | Threads=$THREADS | Repetition=$REP"

        rm -f "$TMP"

        if ! "$BIN" --benchmark \
            --N "$N" --days 200 --seed 42 \
            --threads "$THREADS" --chunks "$C" \
            --mode thread-pool --partition block \
            --benchmark-output "$TMP" \
            > "results/benchmarks/d3_chunks_C${C}_R${REP}.log" 2>&1; then

            cat "results/benchmarks/d3_chunks_C${C}_R${REP}.log"
            exit 1
        fi

        tail -n 1 "$TMP" |
            awk -F',' -v rep="$REP" \
            '{print rep "," $1 "," $5 "," $6 "," $7 "," $8 "," $9 "," $10}' \
            >> "$OUT"

        grep -E "Executed days|Elapsed time|Checksum" \
            "results/benchmarks/d3_chunks_C${C}_R${REP}.log"
    done
done

rm -f "$TMP"

echo "D3.1 experiments completed."
echo "Raw results: $OUT"
