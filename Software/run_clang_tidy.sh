#!/bin/bash
RED='\033[0;31m'
GREEN='\033[0;32m'
NOCOLOR='\033[0m'

# led_wal_master is default folder
TARGET_DIR="${1:-apps/led_wall_master}"
ABS_TARGET_DIR=$(realpath "$TARGET_DIR")
CURRENT_DIR="$(pwd)"

echo -e "${GREEN}Verwende Verzeichnis: ${ABS_TARGET_DIR}${NOCOLOR}"

export IDF_TOOLCHAIN=clang

if [ ! -d "$ABS_TARGET_DIR" ]; then
    echo -e "${RED}Fehler: Verzeichnis $ABS_TARGET_DIR existiert nicht.${NOCOLOR}"
    exit 1
fi

mkdir -p "$ABS_TARGET_DIR/build_clang"
idf.py -C "$ABS_TARGET_DIR" -B "$ABS_TARGET_DIR/build_clang" reconfigure

sed -i 's/-I\/opt\/esp\//-isystem\/opt\/esp\//g' "$ABS_TARGET_DIR/build_clang/compile_commands.json"
echo -e "${GREEN}Running clang-tidy...${NOCOLOR}"

#git ls-files "*.cpp" "*.c" | grep -E "^($ABS_TARGET_DIR/main/|components/)" | xargs -P $(nproc) -I {} clang-tidy {} -p "$ABS_TARGET_DIR/build_clang/" --warnings-as-errors='*' -header-filter="^($ABS_TARGET_DIR/(?!managed_components)main/|components/).*" 2>&1 | tee "$ABS_TARGET_DIR/warnings.txt"

# Ziehe Files die tatsächlich auch kompiliert werden
FILE_FILTER="^($ABS_TARGET_DIR/main/|$CURRENT_DIR/components/)"
HEADER_FILTER="^($ABS_TARGET_DIR/(?!managed_components)main/|$CURRENT_DIR/components/)"

jq -r '.[].file' "$ABS_TARGET_DIR/build_clang/compile_commands.json" | \
grep -E "$FILE_FILTER" | \
xargs -P $(nproc) -I {} clang-tidy {} \
  -p "$ABS_TARGET_DIR/build_clang/" \
  --warnings-as-errors='*' \
  -header-filter="$HEADER_FILTER" \
  2>&1 | tee "$ABS_TARGET_DIR/warnings.txt"

grep "error: " "$ABS_TARGET_DIR/warnings.txt"

ERR_SUM=$(grep -Po '\d+(?= warnings? treated as errors?)' "$ABS_TARGET_DIR/warnings.txt" | awk '{s+=$1} END {print s+0}')

if [ "$ERR_SUM" -gt 0 ]; then
    echo -e "\n${RED}Total Clang-Tidy errors: $ERR_SUM${NOCOLOR}"
    exit 1
else
    echo -e "\n${GREEN}No errors found${NOCOLOR}"
fi