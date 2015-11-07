#!/bin/bash

if [ $1 == "" ]; then
	exit 1
fi

if [ ! -d "../perf_analysis_interlines" ]; then
	exit 1
fi

if [ ! -d "../perf_analysis_interlines/run$1" ]; then
  mkdir ../perf_analysis_interlines/run"$1"
fi

for interlines in 1 2 4 8 16 32 64 128 256 512 1024
do
	salloc -c 12 ../partdiff-openmp 12 2 $interlines 2 2 3000 | grep Berechnungszeit > ../perf_analysis_interlines/run"$1"/result_"$interlines"
done
