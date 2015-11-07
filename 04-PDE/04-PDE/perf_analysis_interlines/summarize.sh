#!/bin/bash

for i in 1 2 4 8 16 32 64 128 256 512 1024; do
	echo -e "$i"\t$(cat ../perf_analysis_interlines/run"$1"/result_"$i") >> ../perf_analysis_interlines/run"$1"/summary
done
