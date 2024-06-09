#!/bin/sh

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
