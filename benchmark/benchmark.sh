#!/bin/bash
readonly script_path="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"
readonly root_path="$script_path/.."
readonly json_file="$script_path/benchmark.json"
readonly csv_file="$script_path/benchmark.csv"

BENCHMARK_OUT="${json_file}" BENCHMARK_OUT_FORMAT=json bazel run //benchmark --//:enable-soplex=true $@

pushd "$script_path" > /dev/null
./benchmark_parser.py "${json_file}" "${csv_file}"
popd > /dev/null
