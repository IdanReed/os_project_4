#!/bin/bash -l
#SBATCH --job-name=proj4pthreads

#SBATCH --mem=4G   # Memory per core, use --mem= for memory per node
#SBATCH --time=0-01:00:00   # Use the form DD-HH:MM:SS
#SBATCH --constraint=elves
#SBATCH --nodes=1

# $1 iter count
# $2 thread count 
# #3 perf out file

for run in $(seq $1)
do 
  ./main $2 $3
done