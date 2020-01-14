#!/bin/bash

# Override default cmake
export PATH="/usr/bin:$PATH"

mkdir -p build && cd build
cmake -DWGTCC_COVERAGE=ON .. && make -j16
cd ../test/ && ./run_tests.sh
