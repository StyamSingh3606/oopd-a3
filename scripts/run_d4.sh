#Satyam Singh
#MT26140
#!/bin/bash

set -e

mkdir -p results/benchmarks

RAW="results/benchmarks/d4_raw.csv"

echo "runs,threads_per_run,repeat,total_seconds,runs_per_second,completed,missing" > "$RAW"

for CONFIG in "1 4" "2 2" "4 1"
do
    read -r R T <<< "$CONFIG"

    for REP in 1 2 3 4 5
    do
        echo "======================================"
        echo "D4: Runs=$R, Threads per run=$T, Repeat=$REP"
        echo "======================================"

        SEEDS=$(seq -s, 1 32)

        OUTPUT="results/benchmarks/d4_R${R}_T${T}_rep${REP}.csv"

        START=$(date +%s.%N)

        ./build-release/pandemic \
            --sweep \
            --N 1000 \
            --days 200 \
            --betas 0.08 \
            --vaccinations 0.0 \
            --seeds "$SEEDS" \
            --workers "$R" \
            --threads-per-run "$T" \
            --time-limit 3600 \
            --sweep-output "$OUTPUT"

        END=$(date +%s.%N)

        ELAPSED=$(awk -v s="$START" -v e="$END" 'BEGIN {printf "%.6f", e-s}')

        COMPLETED=$(($(wc -l < "$OUTPUT") - 1))
        MISSING=$((32 - COMPLETED))

        THROUGHPUT=$(awk -v c="$COMPLETED" -v t="$ELAPSED" 'BEGIN {printf "%.6f", c/t}')

        echo "$R,$T,$REP,$ELAPSED,$THROUGHPUT,$COMPLETED,$MISSING" >> "$RAW"

        echo "Elapsed: $ELAPSED seconds"
        echo "Completed: $COMPLETED / 32"
        echo "Missing: $MISSING"
    done
done

echo "======================================"
echo "D4 EXPERIMENTS COMPLETED"
echo "Raw results: $RAW"
