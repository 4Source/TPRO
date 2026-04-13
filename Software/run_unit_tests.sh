#!/bin/bash
set -e
cd unittest/build_unit
cmake ..
make -j$(nproc)
ctest --output-on-failure