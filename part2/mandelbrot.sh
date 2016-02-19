#!/bin/bash
#$ -N Mandelbrot
#$ -q aparna 
#$ -pe mpi 16
#$ -R y

#-pe one-node-mpi 1
# Grid Engine Notes:
# -----------------
# 1) Use "-R y" to request job reservation otherwise single 1-core jobs
#    may prevent this multicore MPI job from running.   This is called
#    job starvation.

# Module load boost
module load boost/1.57.0

# Module load OpenMPI
module load openmpi-1.8.3/gcc-4.8.3

# Run the program 
#mpirun -np 1  ./mandelbrot_serial 1000 1000
#mpirun -np 4 ./mandelbrot_susie 10000 10000
mpirun -np 2 ./mandelbrot_ms 1000 1000
