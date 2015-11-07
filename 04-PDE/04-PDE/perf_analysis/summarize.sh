#!/bin/bash

for i in 1 2 3 4 5 6 7 8 9 10 11 12; do
	echo -e "$i"\t$(cat ../perf_analysis/run"$1"/result_"$i") >> ../perf_analysis/run"$1"/summary
done
