#!/bin/bash
json_file="/home/ubuntu/dlinear4/benchmark/benchmark.json"
csv_file="/home/ubuntu/dlinear4/benchmark/benchmark.csv"

BENCHMARK_OUT="${json_file}" BENCHMARK_OUT_FORMAT=json bazel run //benchmark --//:enable-soplex=true

./benchmark_parser.py "${json_file}" "${csv_file}"
