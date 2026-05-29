#!/bin/bash
if [ "$#" -ne 2 ]; then
    echo "Error! need two paras"
    echo "Usage: <program> <tinycoro_dir>"
    exit 1
fi

perf record --freq=997 --call-graph dwarf -q -o $2/temp/perf.data $1
perf script -i $2/temp/perf.data &> $2/temp/perf.unfold
$2/third_party/FlameGraph/stackcollapse-perf.pl $2/temp/perf.unfold &> $2/temp/perf.folded
$2/third_party/FlameGraph/flamegraph.pl $2/temp/perf.folded > $2/temp/perf.svg