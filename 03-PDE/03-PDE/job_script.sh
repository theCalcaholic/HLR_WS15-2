#!/bin/bash
#SBATCH --job_name=HLR_jobscript
#SBATCH -o timescript.out
#SBATCH -e jobscript.err
#SBATCH --tasks-per-node=4
##SBATCH -n 4
#SBACH -N 4

srun timescript.sh
srun timescript.sh
srun timescript.sh
srun timescript.sh
echo fertig
