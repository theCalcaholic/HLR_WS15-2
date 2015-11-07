#!/bin/bash

if [ $1 == "" ]; then
  exit 1
fi

if [ ! -d "../perf_analysis" ]; then
	exit 1
fi

mkdir ../perf_analysis/run"$1"

for cpus in 1 2 3 4 5 6 7 8 9 10 11 12
do
  salloc -c $cpus ../partdiff-openmp $cpus 2 512 2 2 256 | grep Berechnungszeit > ../perf_analysis/run"$1"/result_"$cpus"
done
