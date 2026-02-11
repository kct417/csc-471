#!/bin/bash

# Usage: source setenv.sh

export GLFW_DIR="$(pwd)/vendor/glfw"
echo GLFW DIR is set to $GLFW_DIR

export GLM_INCLUDE_DIR="$(pwd)/vendor/glm"
echo GLM INCLUDE DIR is set to $GLM_INCLUDE_DIR
