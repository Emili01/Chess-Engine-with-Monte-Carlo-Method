# Chess-Engine-with-Monte-Carlo-Method
This project implements a chess engine that uses the Monte Carlo method to make move decisions. Additionally, it employs MPI to parallelize simulations and improve efficiency.

# Features 
- Random game simulation to evaluate moves.
- Parallelization with MPI.
- Detection of checkmate, draw, and legal moves.

# Requirements
- C compiler
- MPI (OpenMPI, MPICH, etc.).
- PCG library for random number generation (optional, if used).

To compile it:
mpicc main.c chess.c pcg_basic.c play.c -o chess 

To run it:
mpirun --use-hwthread-cpus -n [number of processes] ./chess [number of simulations per process] [black or white or self]
