#!/bin/sh

# SPDX-License-Identifier: Unlicense

# This is free and unencumbered software released into the public domain.
# For more information, please refer to http://unlicense.org/

set -o errexit -o nounset -o xtrace

mypath="$(realpath "$0")"
basedir="$(dirname "$mypath")/.."

rm -rf "${basedir}/build"/*

cmake \
    -D CMAKE_BUILD_TYPE=Release \
    -D VCPKG_ROOT="${basedir}/vcpkg" \
    -S "${basedir}/MEGAsync" \
    -B "${basedir}/build"

cmake --build "${basedir}/build" --target MEGAsync --parallel $(nproc)
