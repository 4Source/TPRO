#!/bin/bash
export IDF_TOOLCHAIN=clang
#rm -rf build
#idf.py reconfigure
mkdir -p build_clang
# configure with clang and generate compile_commands.json no reconfiguring of build folder needed
idf.py -B build_clang reconfigure
sed -i 's/-I\/opt\/esp\//-isystem\/opt\/esp\//g' build_clang/compile_commands.json
echo "Running clang-tidy..."
git ls-files "*.cpp" "*.c" | grep -E "^(main/|components/)" | xargs clang-tidy -p build_clang/ --warnings-as-errors='*' 2>&1 | tee warnings.txt
ERR_SUM=$(grep -Po '\d+(?= warnings? treated as errors?)' warnings.txt | awk '{s+=$1} END {print s+0}') && [ "$ERR_SUM" -gt 0 ] && echo -e "\nTotal Clang-Tidy errors: $ERR_SUM" && exit 1 || echo -e "\nNo errors found"