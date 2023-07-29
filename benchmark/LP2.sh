#!/bin/bash
readonly script_path="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

pushd "$script_path" > /dev/null
mkdir -p MIPLIB
mkdir -p smt2
popd > /dev/null

pushd "$script_path/MIPLIB" > /dev/null
if [ ! -f "benchmark.zip" ]; then
    wget https://miplib2010.zib.de/download/miplib2010-benchmark.zip -O benchmark.zip
    unzip benchmark.zip
fi
popd > /dev/null
pushd "$script_path/MIPLIB/miplib2010-benchmark" > /dev/null
for file in $(find . -name "*.gz"); do
    gzip -dkf $file
    rm $file
done
for file in $(find . -name "*.mps"); do
    filename="${file%.*}"
    echo "Converting $filename to SMT-LIB2 format..."
    toyconvert "$filename.mps" -o "$filename.smt2"
    sed -i 's/(set-option :produce-models true)/(set-logic QF_LRA)/' "$filename.smt2"
    sed -i 's/(!//' "$filename.smt2"
    sed -i 's/ *:named[^)]*)//' "$filename.smt2"
    rm "$filename.mps"
    rm -f "toyconvert.tix"
done
# popd > /dev/null
# pushd "$script_path" > /dev/null
# for file in $(find mps -name "*.smt2"); do
#     # If the file contains the string Int avoid copying it
#     sed -i 's/Int)/Real)/' "$file"
#     if ! grep -q " Int)" "$file"; then
#         cp $file smt2/LP_$(basename $file)
#     fi
# done
popd > /dev/null