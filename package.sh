#!/bin/sh

# SPDX-License-Identifier: Unlicense

# This is free and unencumbered software released into the public domain.
# For more information, please refer to http://unlicense.org/

set -o errexit -o nounset -o xtrace

appdir=$(mktemp -d -t megasync-appdir.XXXXXXXXXX)
echo "$appdir"

cd MEGAsync/src
make install INSTALL_ROOT="$appdir"

cd -

export QML_SOURCES_PATHS=MEGAsync/src
./linuxdeploy/linuxdeploy-x86_64.AppImage --appdir "$appdir" --executable MEGAsync/src/megasync --plugin qt --output appimage
