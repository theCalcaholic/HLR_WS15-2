#!/bin/bash
##SBATCH --job_name=HLR_jobscript
#SBATCH -o timescript.out
#SBATCH -e jobscript.err
#SBACH -N 4
#SBATCH -n 16
#SBATCH --tasks-per-node=4

srun timescript.sh
echo fertig
