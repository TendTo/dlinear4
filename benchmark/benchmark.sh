json_file="/app/benchmark/benchmark.json"
csv_file="/app/benchmark/benchmark.csv"

BENCHMARK_OUT="${json_file}" BENCHMARK_OUT_FORMAT=json bazel run //benchmark
# When soplex support is added
# BENCHMARK_OUT="${json_file}" BENCHMARK_OUT_FORMAT=json bazel run //benchmark --//:enable-soplex=true
# BENCHMARK_OUT="/app/benchmarks.txt" BENCHMARK_OUT_FORMAT=csv bazel run //benchmark --//:enable-soplex=true --cxxopt=-fext-numeric-literals --define SOPLEX_WITH_BOOST

./benchmark_parser.py "${json_file}" "${csv_file}"
