#!/bin/bash

# Wir geben dem job einen netten Namen
#SBATCH --job-name=HLR_jobscript

# stdout soll in timescript.out geschrieben werden
#SBATCH -o timescript.out

# stderr soll in jobscript.err geschrieben werden
#SBATCH -e jobscript.err

# Wir fordern 4 Nodes an
#SBATCH -N 4

# Der Task soll 16 mal insgesamt ausgeführt werden
#SBATCH -n 16

# Und dabei gleichmäßig auf die 4 Nodes aufgeteilt werden.
#SBATCH --tasks-per-node=4

srun timescript.sh

# "fertig" soll nicht in timescript.out geschrieben werden, sondern in job_script.out
echo fertig > job_script.out
