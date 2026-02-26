#!/bin/bash

# Usage: ./run.sh <project>

source build.sh $1
./${1%/}
