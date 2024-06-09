#!/bin/sh

# SPDX-License-Identifier: Unlicense

# This is free and unencumbered software released into the public domain.
# For more information, please refer to http://unlicense.org/

set -o errexit -o nounset -o xtrace

sudo apt-get -y update

sudo apt-get -y install --no-install-recommends \
    build-essential wget dh-autoreconf cdbs unzip libtool-bin pkg-config debhelper \
    qttools5-dev-tools qtbase5-dev qt5-qmake libqt5x11extras5-dev libqt5dbus5 \
    libqt5svg5-dev qtdeclarative5-dev qml-module-qtquick-dialogs qml-module-qtquick-controls qml-module-qtquick-controls2 \
    libcrypto++-dev libraw-dev libc-ares-dev libssl-dev sqlite3 libsqlite3-dev zlib1g-dev \
    libavcodec-dev libavutil-dev libavformat-dev libswscale-dev mediainfo libfreeimage-dev \
    libreadline-dev libsodium-dev libuv1 libuv1-dev libudev-dev libzen-dev libx11-dev libx11-xcb-dev libgl-dev \
    libz-dev libicu-dev libmediainfo-dev \
    fuse
