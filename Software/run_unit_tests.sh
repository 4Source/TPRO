#!/bin/bash
set -e
mkdir -p unittest/build_unit
cd unittest/build_unit
cmake ..
make -j$(nproc)
ctest --output-on-failure