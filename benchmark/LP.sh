#!/bin/bash
readonly script_path="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

pushd "$script_path" > /dev/null
mkdir -p mps
popd > /dev/null

pushd "$script_path/mps" > /dev/null
# wget https://miplib.zib.de/downloads/benchmark.zip
# unzip benchmark.zip
for file in $(find . -name "*.gz"); do
    readonly filename="${file%%.*}"
    gzip -dkf $file
    rm $file
    toyconvert "$filename.mps" -o "$filename.smt2"
    sed -i 's/(set-option :produce-models true)/(set-logic QF_LRA)/' "$filename.smt2"
    sed -i 's/(!//' "$filename.smt2"
    sed -i 's/ *:named[^)]*)//' "$filename.smt2"
    rm "$filename.mps"
    rm -f "toyconvert.tix"
done
popd > /dev/null