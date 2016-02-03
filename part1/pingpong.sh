#!/bin/bash
#$ -q aparna
#$ -pe mpi 16
#$ -N pingpong
#$ -R y

date
hostname
echo -e "\n\n"

# Module load OpenMPI
module load openmpi-1.8.3/gcc-4.8.3

# Run the ping-pong benchmark
mpirun -np 2 -npernode 1 ./pingpong

# Generate netplot.png
gnuplot netplot.gnu

# eof
