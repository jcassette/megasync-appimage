#!/bin/sh
set -e
#set -u

jflag=
jval=1
rebuild=0
download_only=0

while getopts 'j:df' OPTION
do
  case $OPTION in
  j)
      jflag=1
      jval="$OPTARG"
      ;;
  f)
      fake=1
      ;;
  d)
      download_only=1
      ;;
  ?)
      printf "Usage: %s: [-j concurrency_level] (hint: your cores + 20%%) [-B] [-d]\n" $(basename $0) >&2
      exit 2
      ;;
  esac
done
shift $(($OPTIND - 1))

if [ "$jflag" ]
then
  if [ "$jval" ]
  then
    printf "Option -j specified (%d)\n" $jval
  fi
fi

cd `dirname $0`
ENV_ROOT=`pwd`
. ./env.source

echo "#### Qt build ####"
##this is our working directory
#cd $BUILD_DIR

ver=5.15.13
mainver=`echo $ver | awk -F"." '{print $1"."$2}'`

#download and extract package
if [ ! -f src.tar.xz ]; then
wget https://download.qt.io/official_releases/qt/$mainver/$ver/single/qt-everywhere-opensource-src-$ver.tar.xz -O src.tar.xz
fi

# Get patches
mkdir -p patches
for p in \
CVE-2023-32573-qtsvg-5.15.diff \
CVE-2023-32762-qtbase-5.15.diff \
CVE-2023-32763-qtbase-5.15.diff \
CVE-2023-33285-qtbase-5.15.diff \
CVE-2023-34410-qtbase-5.15.diff \
CVE-2023-37369-qtbase-5.15.diff \
CVE-2023-38197-qtbase-5.15.diff \
CVE-2023-43114-5.15.patch \
0001-CVE-2023-51714-qtbase-5.15.diff \
0002-CVE-2023-51714-qtbase-5.15.diff \
CVE-2024-25580-qtbase-5.15.diff
do
  if [ ! -f patches/$p ]; then
    wget https://download.qt.io/official_releases/qt/5.15/$p -O patches/$p
  fi
done

[ $download_only -eq 1 ] && exit 0
echo "CHECKSUM ... "
if ! echo 9550ec8fc758d3d8d9090e261329700ddcd712e2dda97e5fcfeabfac22bea2ca src.tar.xz | sha256sum -c - \
|| ! echo b9fb5999c4e2e9dca4032db124c76da974d2b4f5026447b41bd77fc042469702 patches/CVE-2023-32573-qtsvg-5.15.diff | sha256sum -c - \
|| ! echo bb35307cd093b39c71242be8a55882e21c0fe5863d2ffb5c8ae6b9aca537ce76 patches/CVE-2023-32762-qtbase-5.15.diff | sha256sum -c - \
|| ! echo e8acecaea4d0e78408fc7049c44c7fb44804d459e7c7f700e08fe0b7d4052256 patches/CVE-2023-32763-qtbase-5.15.diff | sha256sum -c - \
|| ! echo 7a0b40dd183dc23317afd1e2b15487686cb6c38bf1eca93aa499a26a456b0224 patches/CVE-2023-33285-qtbase-5.15.diff | sha256sum -c - \
|| ! echo 01917eae0587b72f1969c303289a26bc1a148fea2eef7a64e05e6e86c1dcc178 patches/CVE-2023-34410-qtbase-5.15.diff | sha256sum -c - \
|| ! echo 279c520ec96994d2b684ddd47a4672a6fdfc7ac49a9e0bdb719db1e058d9e5c0 patches/CVE-2023-37369-qtbase-5.15.diff | sha256sum -c - \
|| ! echo 382c10ec8f42e2a34ac645dc4f57cd6b717abe6a3807b7d5d9312938f91ce3dc patches/CVE-2023-38197-qtbase-5.15.diff | sha256sum -c - \
|| ! echo 14cc26aa465ec9a5cac6f6b4c91b2f802b12a8134a6ab897a45449c418ca98c1 patches/CVE-2023-43114-5.15.patch | sha256sum -c - \
|| ! echo 2129058a5e24d98ee80a776c49a58c2671e06c338dffa7fc0154e82eef96c9d4 patches/0001-CVE-2023-51714-qtbase-5.15.diff | sha256sum -c - \
|| ! echo 99d5d32527e767d6ab081ee090d92e0b11f27702619a4af8966b711db4f23e42 patches/0002-CVE-2023-51714-qtbase-5.15.diff | sha256sum -c - \
|| ! echo 7cc9bf74f696de8ec5386bb80ce7a2fed5aa3870ac0e2c7db4628621c5c1a731 patches/CVE-2024-25580-qtbase-5.15.diff | sha256sum -c -

then
exit 1
fi

echo "EXTRACT SOURCES ... "
mkdir -p src && tar xvf src.tar.xz -C src --strip-components=1

# Apply patches
echo "APPLY PATCHES ... "
for p in \
CVE-2023-32762-qtbase-5.15.diff \
CVE-2023-32763-qtbase-5.15.diff \
CVE-2023-33285-qtbase-5.15.diff \
CVE-2023-34410-qtbase-5.15.diff \
CVE-2023-37369-qtbase-5.15.diff \
CVE-2023-38197-qtbase-5.15.diff \
CVE-2023-43114-5.15.patch \
0001-CVE-2023-51714-qtbase-5.15.diff \
0002-CVE-2023-51714-qtbase-5.15.diff \
CVE-2024-25580-qtbase-5.15.diff
do
  if ! patch -f --verbose -p1 -d src/qtbase < patches/$p ; then
    exit 1
  fi
done

for p in \
CVE-2023-32573-qtsvg-5.15.diff
do
  if ! patch -f --verbose -p1 -d src/qtsvg < patches/$p ; then
    exit 1
  fi
done

# Build
echo "BUILD ... "
mkdir -p build && cd build
[ -f config.cache ] && rm config.cache; 

../src/configure --prefix=$TARGET_DIR/opt/mega \
-opensource -ssl -confirm-license -nomake examples -nomake tests -nomake tools \
-skip qtwebview -skip qtwebengine -skip qtwebchannel -skip qtconnectivity -skip qt3d \
-skip qtlocation -skip qtvirtualkeyboard \
-fontconfig 2>&1 | tee salconfigure.txt

make -j $jval
make install
