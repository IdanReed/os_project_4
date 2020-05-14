#!/bin/bash -l

echo "Running $1 iterations"
for run in 1 4 6 8 10 12 14 16 18 20
do
  sbatch --cpus-per-task=$run --output=$run"_output.out" run_ompthread.sh $1 $run "perf_out.csv"
done
