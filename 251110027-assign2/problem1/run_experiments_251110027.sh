#!/bin/bash

# Build the executables
echo "Compiling programs..."
make clean
make all

# Configuration
MAX_THREADS=32
STRONG_TOTAL_OPS=10000    # For Strong scaling, total work is constant
WEAK_OPS_PER_THREAD=5000   # For Weak scaling, work per thread is constant

OUT_FILE="results.csv"
echo "Scaling,Program,Lock,Threads,TotalOps,Time_ms" > $OUT_FILE

PROGRAMS=("sum" "vec")
LOCKS=("pthreads" "abq" "mcs")

# 1. Strong Scaling Experiments
echo "Starting Strong Scaling Tests..."
for prog in "${PROGRAMS[@]}"; do
    for lock in "${LOCKS[@]}"; do
        exec_name="${lock}_${prog}"
        for t in $(seq 1 $MAX_THREADS); do
            # Run program and capture output
            output=$(./$exec_name $t $STRONG_TOTAL_OPS)
            # Extract the numerical time value from the line "  Time     = X.XXX ms"
            time_ms=$(echo "$output" | grep "Time" | awk '{print $3}')
            
            echo "Strong,$prog,$lock,$t,$STRONG_TOTAL_OPS,$time_ms" >> $OUT_FILE
            echo "  [Strong] $exec_name | Threads: $t | Time: $time_ms ms"
        done
    done
done

# 2. Weak Scaling Experiments
echo "Starting Weak Scaling Tests..."
for prog in "${PROGRAMS[@]}"; do
    for lock in "${LOCKS[@]}"; do
        exec_name="${lock}_${prog}"
        for t in $(seq 1 $MAX_THREADS); do
            # Total ops increases with number of threads
            total_ops=$(( t * WEAK_OPS_PER_THREAD ))
            
            # Run program and capture output
            output=$(./$exec_name $t $total_ops)
            # Extract the numerical time value
            time_ms=$(echo "$output" | grep "Time" | awk '{print $3}')
            
            echo "Weak,$prog,$lock,$t,$total_ops,$time_ms" >> $OUT_FILE
            echo "  [Weak] $exec_name | Threads: $t | Time: $time_ms ms"
        done
    done
done

echo "Experiments complete! Results saved to $OUT_FILE"
echo "You can use this CSV file to create plots for strong and weak scaling."
echo "Run 'python3 plot_results_251110027.py' to generate the plots from the results."