#!/bin/bash

set -e

OUT_DIR="results/part-e/interventions"
mkdir -p "$OUT_DIR"

N=500
DAYS=1000
BETA=0.08

run_experiment() {
    local name=$1
    local mask=$2
    local lambda=$3
    local theta_on=$4
    local theta_off=$5

    echo ""
    echo "======================================"
    echo "Configuration: $name"
    echo "======================================"

    for seed in $(seq 1 10); do
        output="$OUT_DIR/${name}_seed${seed}.csv"

        echo "Running $name | Seed $seed / 10"

        ./build/pandemic \
            --N "$N" \
            --days "$DAYS" \
            --seed "$seed" \
            --mode thread-pool \
            --partition block \
            --threads 4 \
            --beta "$BETA" \
            --v 0 \
            --mask "$mask" \
            --lambda "$lambda" \
            --theta-on "$theta_on" \
            --theta-off "$theta_off" \
            --output "$output" \
            > /dev/null
    done
}

# Mask experiments: lockdown disabled
run_experiment "no_intervention" 1.0 1.0 1.0 0.0
run_experiment "mild_mask"       0.8 1.0 1.0 0.0
run_experiment "strong_mask"     0.5 1.0 1.0 0.0

# Dynamic lockdown experiments
run_experiment "mild_lockdown"   1.0 0.7 0.02 0.005
run_experiment "strong_lockdown" 1.0 0.3 0.02 0.005

# Combined mask and lockdown
run_experiment "combined"        0.5 0.3 0.02 0.005

echo ""
echo "All intervention experiments completed!"
