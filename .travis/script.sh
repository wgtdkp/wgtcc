#!/bin/bash

# Override default cmake
export PATH="/usr/bin:$PATH"

cmake . -Bbuild
cd build && make
cd ../test/ && ./run_tests.sh
