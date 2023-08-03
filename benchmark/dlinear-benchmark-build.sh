#!/bin/bash
readonly script_path="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"
readonly root_path="$script_path/.."

pushd "$root_path" > /dev/null

rm -rf benchmark.runfiles
bazel build //benchmark --//:enable-soplex=true
cp -rL bazel-bin/benchmark/benchmark.runfiles .
mv benchmark.runfiles/dreal/* benchmark.runfiles/
rm -rf benchmark.runfiles/dreal
cp install/qsopt-ex/lib/libqsopt_ex.so.2 benchmark.runfiles/_solib_k8
tar -czf benchmark.runfiles.tar.gz benchmark.runfiles

popd > /dev/null
