mkdir -p ../smt2_random
ls |sort -R |tail -100 |while read file; do
    echo "Converting $file to ../smt2_random/$file.smt2"
    mv "$file" "../smt2_random/$file.smt2"
done