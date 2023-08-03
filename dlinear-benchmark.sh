#!/bin/bash
readonly script_path="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"
readonly root_path="$script_path/.."
readonly json_file="$script_path/benchmark.json"
readonly csv_file="$script_path/benchmark.csv"

export BENCHMARK_OUT="${json_file}" 
export BENCHMARK_OUT_FORMAT=json
export LD_LIBRARY_PATH="$script_path/benchmark.runfiles/libs"
"$script_path/benchmark.runfiles/benchmark/benchmark" $@
