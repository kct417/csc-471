#!/bin/bash

# Usage: source build.sh <project>

source setenv.sh
cd $1
mkdir -p build
cd build
cmake ..
make
