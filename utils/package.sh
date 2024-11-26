#!/bin/sh

# SPDX-License-Identifier: Unlicense

# This is free and unencumbered software released into the public domain.
# For more information, please refer to http://unlicense.org/

set -o errexit -o nounset -o xtrace

mypath="$(realpath "$0")"
basedir="$(dirname "$mypath")/.."

rm -rf "${basedir}/appdir"/*

cmake --install "${basedir}/build" --prefix "${basedir}/appdir"

export QML_SOURCES_PATHS="${basedir}/megadesktop/src"
export LD_LIBRARY_PATH="${basedir}/appdir/opt/megasync/lib"
"${basedir}"/linuxdeploy/linuxdeploy-x86_64.AppImage \
    --appdir "${basedir}/appdir" \
    --plugin qt \
    --output appimage
