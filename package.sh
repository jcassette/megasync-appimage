#!/bin/sh

set -o errexit -o nounset -o xtrace

appdir=$(mktemp -d -t megasync-appdir.XXXXXXXXXX)
echo "$appdir"

cd MEGAsync/src
make install INSTALL_ROOT="$appdir"

cd -

export QML_SOURCES_PATHS=MEGAsync/src
./linuxdeploy-x86_64.AppImage --appdir "$appdir" -e MEGAsync/src/megasync --plugin qt --output appimage
