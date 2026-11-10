
#!/bin/bash

set -e

mkdir -p results/part-e/vaccination

VACCINATIONS=(0.0 0.1 0.2 0.3 0.4 0.5 0.6 0.7 0.8 0.9)

for v in "${VACCINATIONS[@]}"; do
    echo "======================================"
    echo "Running vaccination = $v"
    echo "======================================"

    for seed in {1..10}; do
        output="results/part-e/vaccination/v_${v}_seed_${seed}.csv"

        echo "Vaccination=$v | Seed=$seed"

        ./build/pandemic \
            --N 500 \
            --days 1000 \
            --seed "$seed" \
            --mode thread-pool \
            --partition block \
            --threads 4 \
            --beta 0.08 \
            --v "$v" \
            --mask 1 \
            --lambda 1 \
            --theta-on 1 \
            --theta-off 0 \
            --output "$output" \
            > /dev/null
    done
done

echo "Vaccination sweep completed successfully."
