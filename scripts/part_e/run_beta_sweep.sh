
#!/bin/bash

set -e

mkdir -p results/part-e/beta

BETAS=(0.02 0.04 0.06 0.08 0.10 0.12)

for beta in "${BETAS[@]}"; do
    echo "======================================"
    echo "Running beta = $beta"
    echo "======================================"

    for seed in {1..10}; do
        output="results/part-e/beta/beta_${beta}_seed_${seed}.csv"

        echo "Beta=$beta | Seed=$seed"

        ./build/pandemic \
            --N 500 \
            --days 1000 \
            --seed "$seed" \
            --mode thread-pool \
            --partition block \
            --threads 4 \
            --beta "$beta" \
            --v 0 \
            --mask 1 \
            --lambda 1 \
            --theta-on 1 \
            --theta-off 0 \
            --output "$output" \
            > /dev/null
    done
done

echo "Beta sweep completed successfully."
