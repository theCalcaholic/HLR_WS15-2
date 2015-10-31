#!/bin/bash
workingDir="/home/knoeppler/HLR/HLR_WS15-2/03-PDE/03-PDE"

#SBATCH --job_name=HLR_jobscript
#SBATCH -o timescript.out
#SBATCH -e jobscript.err
#SBATCH --tasks-per-node=1
#SBATCH -n 4
#SBACH -N 4

cd $workingDir
echo $(ls)
srun ./timescript.sh
srun ./timescript.sh
srun ./timescript.sh
srun ./timescript.sh
srun ./timescript.sh
srun ./timescript.sh
srun ./timescript.sh
srun ./timescript.sh
echo fertig
