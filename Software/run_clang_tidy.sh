#!/bin/bash
RED='\033[0;31m'
GREEN='\033[0;32m'
NOCOLOR='\033[0m'
export IDF_TOOLCHAIN=clang
#rm -rf build
#idf.py reconfigure
mkdir -p build_clang
# configure with clang and generate compile_commands.json no reconfiguring of build folder needed
idf.py -B build_clang reconfigure
sed -i 's/-I\/opt\/esp\//-isystem\/opt\/esp\//g' build_clang/compile_commands.json
echo -e "${GREEN}Running clang-tidy...${NOCOLOR}"
git ls-files "*.cpp" "*.c" | grep -E "^(main/|components/)" | xargs -P $(nproc) -I {} clang-tidy {} -p build_clang/ --warnings-as-errors='*' 2>&1 | tee warnings.txt
echo -e "\n${RED}Summary of Clang-Tidy warnings treated as errors:${NOCOLOR}"
cat warnings.txt | grep "error: "
ERR_SUM=$(grep -Po '\d+(?= warnings? treated as errors?)' warnings.txt | awk '{s+=$1} END {print s+0}') && [ "$ERR_SUM" -gt 0 ] && echo -e "\n${RED}Total Clang-Tidy errors: $ERR_SUM${NOCOLOR}" && exit 1 || echo -e "\n${GREEN}No errors found${NOCOLOR}"
