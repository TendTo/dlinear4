#!/bin/bash
readonly script_path="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

pushd "$script_path" > /dev/null
mkdir -p mps
mkdir -p smt2
popd > /dev/null

pushd "$script_path/mps" > /dev/null
if [ ! -f "benchmark.zip" ]; then
    wget https://miplib.zib.de/downloads/benchmark.zip
    unzip benchmark.zip
fi
# for file in $(find . -name "*.gz"); do
#     filename="${file%.*}"
#     filename="${filename%.*}"
#     gzip -dkf $file
#     rm $file
#     echo "Converting $filename to SMT-LIB2 format..."
#     toyconvert "$filename.mps" -o "$filename.smt2"
#     sed -i 's/(set-option :produce-models true)/(set-logic QF_LRA)/' "$filename.smt2"
#     sed -i 's/(!//' "$filename.smt2"
#     sed -i 's/Int)/Real)/' "$filename.smt2"
#     sed -i 's/ *:named[^)]*)//' "$filename.smt2"
#     rm "$filename.mps"
#     rm -f "toyconvert.tix"
# done
# popd > /dev/null
pushd "$script_path" > /dev/null
for file in $(find mps -name "*.smt2"); do
    # If the file contains the string Int avoid copying it
    sed -i 's/Int)/Real)/' "$file"
    if ! grep -q " Int)" "$file"; then
        cp $file smt2/LP_$(basename $file)
    fi
done
popd > /dev/null