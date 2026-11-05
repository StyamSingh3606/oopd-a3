#!/bin/bash
set -euo pipefail

BIN="./build-release/pandemic"
OUT="results/benchmarks/d3_partition_raw.csv"
TMP="results/benchmarks/d3_temp.csv"

mkdir -p results/benchmarks
rm -f "$OUT" "$TMP"

echo "repetition,N,threads,chunks,mode,partition,elapsed_seconds,checksum" > "$OUT"

PARTITIONS=(row block interleaved)
REPETITIONS=5

for PART in "${PARTITIONS[@]}"; do
    for REP in $(seq 1 "$REPETITIONS"); do
        echo "D3.2: Partition=$PART | Repetition=$REP"

        rm -f "$TMP"

        if ! "$BIN" --benchmark \
            --N 4000 --days 200 --seed 42 \
            --threads 4 --chunks 16 \
            --mode thread-pool --partition "$PART" \
            --benchmark-output "$TMP" \
            > "results/benchmarks/d3_partition_${PART}_R${REP}.log" 2>&1; then

            cat "results/benchmarks/d3_partition_${PART}_R${REP}.log"
            exit 1
        fi

        tail -n 1 "$TMP" |
            awk -F',' -v rep="$REP" \
            '{print rep "," $1 "," $5 "," $6 "," $7 "," $8 "," $9 "," $10}' \
            >> "$OUT"

        grep -E "Executed days|Elapsed time|Checksum" \
            "results/benchmarks/d3_partition_${PART}_R${REP}.log"
    done
done

rm -f "$TMP"

echo "D3.2 experiments completed."
echo "Raw results: $OUT"
