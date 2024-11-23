#!/bin/sh

# SPDX-License-Identifier: Unlicense

# This is free and unencumbered software released into the public domain.
# For more information, please refer to http://unlicense.org/

set -o errexit -o nounset -o xtrace

apt-get -y update

apt-get -y install --no-install-recommends \
    build-essential \
    ca-certificates \
    git \
    cmake \
    wget \
    autoconf-archive \
    autoconf \
    automake \
    curl \
    zip \
    unzip \
    tar \
    pkg-config \
    nasm

apt-get -y install --no-install-recommends \
    libxcb-cursor0 \
    qtbase5-dev \
    qttools5-dev  \
    libqt5x11extras5-dev  \
    libqt5svg5-dev    \
    qtdeclarative5-dev \
    qml-module-qtquick-dialogs \
    qml-module-qtquick-controls2

apt-get -y install fuse
