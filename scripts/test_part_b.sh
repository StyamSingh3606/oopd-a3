#Satyam Singh
#MT26140
#!/usr/bin/env bash
set -euo pipefail

EXE="./build/pandemic"
N=100
DAYS=50
SEED=42
CHUNKS=16

REFERENCE="results/sequential.csv"

mkdir -p results

echo "Building sequential reference..."

"$EXE" \
    --N "$N" \
    --days "$DAYS" \
    --seed "$SEED" \
    --mode sequential \
    --output "$REFERENCE"

THREAD_COUNTS=(1 2 3 4 7 8 16)
MODES=("thread-per-day" "thread-pool")
PARTITIONS=("row" "block" "interleaved")

TOTAL=0
PASSED=0

for MODE in "${MODES[@]}"; do
    for PARTITION in "${PARTITIONS[@]}"; do
        for T in "${THREAD_COUNTS[@]}"; do

            TOTAL=$((TOTAL + 1))

            OUTPUT="results/${MODE}_${PARTITION}_T${T}.csv"

            echo ""
            echo "Testing: mode=$MODE partition=$PARTITION threads=$T"

            "$EXE" \
                --N "$N" \
                --days "$DAYS" \
                --seed "$SEED" \
                --mode "$MODE" \
                --partition "$PARTITION" \
                --threads "$T" \
                --chunks "$CHUNKS" \
                --output "$OUTPUT"

            if diff -q "$REFERENCE" "$OUTPUT" > /dev/null; then
                echo "PASS"
                PASSED=$((PASSED + 1))
            else
                echo "FAIL"
                echo "Mismatch found in $OUTPUT"
                exit 1
            fi

        done
    done
done

echo ""
echo "================================"
echo "Part B Verification Summary"
echo "================================"
echo "Total tests: $TOTAL"
echo "Passed: $PASSED"
echo "Failed: $((TOTAL - PASSED))"
