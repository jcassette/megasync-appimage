#!/bin/sh

# SPDX-License-Identifier: Unlicense

# This is free and unencumbered software released into the public domain.
# For more information, please refer to http://unlicense.org/

set -o errexit -o nounset -o xtrace

# Configure the MEGA SDK
cd MEGAsync/src/MEGASync/mega
./autogen.sh
./configure

cd -

# Build the Desktop app
cd MEGAsync/src
qmake MEGASync/MEGASync.pro
lrelease MEGASync/MEGASync.pro
make -j $(nproc)
