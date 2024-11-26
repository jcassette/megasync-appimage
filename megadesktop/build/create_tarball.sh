#!/bin/bash

##
 # @file build/create_tarball.sh
 # @brief Generates MEGAsync and extensions tarballs
 #
 # (c) 2013-2014 by Mega Limited, Auckland, New Zealand
 #
 # This file is part of the MEGA SDK - Client Access Engine.
 #
 # Applications using the MEGA API must present a valid application key
 # and comply with the the rules set forth in the Terms of Service.
 #
 # The MEGA SDK is distributed in the hope that it will be useful,
 # but WITHOUT ANY WARRANTY; without even the implied warranty of
 # MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 #
 # @copyright Simplified (2-clause) BSD License.
 #
 # You should have received a copy of the license along with this
 # program.
##

set -euo pipefail
IFS=$'\n\t'

# get current version
MEGASYNC_VERSION=`grep -Po '#define VER_PRODUCTVERSION_STR[[:space:]]*"\K.*(?=\.)' ../src/MEGASync/control/Version.h`
export MEGASYNC_NAME=megasync-$MEGASYNC_VERSION
rm -rf $MEGASYNC_NAME.tar.gz
rm -rf $MEGASYNC_NAME

echo "MEGAsync version: $MEGASYNC_VERSION"

# delete previously generated files
rm -fr MEGAsync/MEGAsync/megasync*.dsc

# fix version number in template files and copy to appropriate directories
sed -e "s/MEGASYNC_VERSION/$MEGASYNC_VERSION/g" templates/MEGAsync/megasync.spec | sed "s#^ *##g" > MEGAsync/MEGAsync/megasync.spec
sed -e "s/MEGASYNC_VERSION/$MEGASYNC_VERSION/g" templates/MEGAsync/megasync.dsc > MEGAsync/MEGAsync/megasync.dsc
sed -e "s/MEGASYNC_VERSION/$MEGASYNC_VERSION/g" templates/MEGAsync/PKGBUILD > MEGAsync/MEGAsync/PKGBUILD
for dscFile in `find templates/MEGAsync/ -name megasync-xUbuntu_* -o -name megasync-Debian_* -o -name megasync-Raspbian_*`; do
    sed -e "s/MEGASYNC_VERSION/$MEGASYNC_VERSION/g" "${dscFile}" > MEGAsync/MEGAsync/`basename ${dscFile}`
done

#include license as copyright file
echo "Format: http://www.debian.org/doc/packaging-manuals/copyright-format/1.0/" > MEGAsync/MEGAsync/debian.copyright
echo "Upstream-Name: megasync" >> MEGAsync/MEGAsync/debian.copyright
echo "Upstream-Contact: <support@mega.nz>" >> MEGAsync/MEGAsync/debian.copyright
echo "Source: https://github.com/meganz/MEGAsync" >> MEGAsync/MEGAsync/debian.copyright
echo "Files: *" >> MEGAsync/MEGAsync/debian.copyright
echo "Copyright: 2013, Mega Limited" >> MEGAsync/MEGAsync/debian.copyright
echo -n "License:" >> MEGAsync/MEGAsync/debian.copyright # Some software (e.g: gnome-software) would only recognized these licenses: http://spdx.org/licenses/
cat ../LICENCE.md | sed 's#^\s*$#\.#g' | sed 's#^# #' >> MEGAsync/MEGAsync/debian.copyright
cat ../LICENCE.md | sed 's#^\s*$#\.#g' | sed 's#^# #' >> MEGAsync/MEGAShellExtDolphin/debian.copyright
cat ../LICENCE.md | sed 's#^\s*$#\.#g' | sed 's#^# #' >> MEGAsync/MEGAShellExtNautilus/debian.copyright
cat ../LICENCE.md | sed 's#^\s*$#\.#g' | sed 's#^# #' >> MEGAsync/MEGAShellExtNemo/debian.copyright
cat ../LICENCE.md | sed 's#^\s*$#\.#g' | sed 's#^# #' >> MEGAsync/MEGAShellExtThunar/debian.copyright

# read the last generated ChangeLog version
version_file="version"

if [ -s "$version_file" ]; then
    last_version=$(cat "$version_file")
else
    last_version="none"
fi

if [ "$last_version" != "$MEGASYNC_VERSION" ]; then
    # add RPM ChangeLog entry
    changelog="MEGAsync/MEGAsync/megasync.changes"
    changelogold="MEGAsync/MEGAsync/megasync.changes.old"
    if [ -f $changelog ]; then
        mv $changelog $changelogold
    fi
    ./generate_rpm_changelog_entry.sh ../src/MEGASync/control/Version.h > $changelog
    if [ -f $changelogold ]; then
        cat $changelogold >> $changelog
        rm $changelogold
    fi

    # add DEB ChangeLog entry
    changelog="MEGAsync/MEGAsync/debian.changelog"
    changelogold="MEGAsync/MEGAsync/debian.changelog.old"
    if [ -f $changelog ]; then
        mv $changelog $changelogold
    fi
    ./generate_deb_changelog_entry.sh $MEGASYNC_VERSION ../src/MEGASync/control/Version.h > $changelog
    if [ -f $changelogold ]; then
        cat $changelogold >> $changelog
        rm $changelogold
    fi

    # update version file
    echo $MEGASYNC_VERSION > $version_file
fi

# create archive
mkdir $MEGASYNC_NAME
ln -s ../MEGAsync/MEGAsync/megasync.spec $MEGASYNC_NAME/megasync.spec
ln -s ../MEGAsync/MEGAsync/debian.postinst $MEGASYNC_NAME/debian.postinst
ln -s ../MEGAsync/MEGAsync/debian.prerm $MEGASYNC_NAME/debian.prerm
ln -s ../MEGAsync/MEGAsync/debian.postrm $MEGASYNC_NAME/debian.postrm
ln -s ../MEGAsync/MEGAsync/debian.copyright $MEGASYNC_NAME/debian.copyright
ln -s ../../CMakeLists.txt $MEGASYNC_NAME/CMakeLists.txt
ln -s ../../vcpkg.json $MEGASYNC_NAME/vcpkg.json
mkdir $MEGASYNC_NAME/contrib
ln -s ../../../contrib/cmake $MEGASYNC_NAME/contrib/
mkdir $MEGASYNC_NAME/src
ln -s ../../../src/CMakeLists.txt $MEGASYNC_NAME/src/CMakeLists.txt
ln -s ../../../src/MEGASync $MEGASYNC_NAME/src/

tar czfh $MEGASYNC_NAME.tar.gz --exclude-vcs $MEGASYNC_NAME
rm -rf $MEGASYNC_NAME

# delete any previous archive
rm -fr MEGAsync/MEGAsync/megasync_*.tar.gz
# transform arch name, to satisfy Debian requirements
mv $MEGASYNC_NAME.tar.gz MEGAsync/MEGAsync/megasync_$MEGASYNC_VERSION.tar.gz

#get md5sum and replace in PKGBUILD
MD5SUM=`md5sum MEGAsync/MEGAsync/megasync_$MEGASYNC_VERSION.tar.gz | awk '{print $1}'`
sed "s/MD5SUM/$MD5SUM/g"  -i MEGAsync/MEGAsync/PKGBUILD



#
# Nautilus
#

# make sure the source tree is in "clean" state
cd ../src/MEGAShellExtNautilus/
make distclean 2> /dev/null || true
cd ../../build

#Get current version
export EXT_VERSION=`cat MEGAsync/MEGAShellExtNautilus/debian.changelog | head -1 | sed -e 's#[^(]*(\([^)]*\))[^)]*#\1#g'`
export EXT_NAME=nautilus-megasync-$EXT_VERSION
rm -rf $EXT_NAME.tar.gz
rm -rf $EXT_NAME

# delete previously generated files
rm -fr MEGAsync/MEGAShellExtNautilus/nautilus-megasync_*.dsc

# fix version number in template files and copy to appropriate directories
sed -e "s/EXT_VERSION/$EXT_VERSION/g" templates/MEGAShellExtNautilus/nautilus-megasync.spec > MEGAsync/MEGAShellExtNautilus/nautilus-megasync.spec
sed -e "s/EXT_VERSION/$EXT_VERSION/g" templates/MEGAShellExtNautilus/nautilus-megasync.dsc > MEGAsync/MEGAShellExtNautilus/nautilus-megasync_$EXT_VERSION.dsc
sed -e "s/EXT_VERSION/$EXT_VERSION/g" templates/MEGAShellExtNautilus/PKGBUILD > MEGAsync/MEGAShellExtNautilus/PKGBUILD

# create archive
mkdir $EXT_NAME
ln -s ../MEGAsync/MEGAShellExtNautilus/nautilus-megasync.spec $EXT_NAME/nautilus-megasync.spec
ln -s ../MEGAsync/MEGAShellExtNautilus/debian.postinst $EXT_NAME/debian.postinst
ln -s ../../src/MEGAShellExtNautilus/mega_ext_client.c $EXT_NAME/mega_ext_client.c
ln -s ../../src/MEGAShellExtNautilus/mega_ext_client.h $EXT_NAME/mega_ext_client.h
ln -s ../../src/MEGAShellExtNautilus/mega_ext_module.c $EXT_NAME/mega_ext_module.c
ln -s ../../src/MEGAShellExtNautilus/mega_notify_client.h $EXT_NAME/mega_notify_client.h
ln -s ../../src/MEGAShellExtNautilus/mega_notify_client.c $EXT_NAME/mega_notify_client.c
ln -s ../../src/MEGAShellExtNautilus/MEGAShellExt.c $EXT_NAME/MEGAShellExt.c
ln -s ../../src/MEGAShellExtNautilus/MEGAShellExt.h $EXT_NAME/MEGAShellExt.h
ln -s ../../src/MEGAShellExtNautilus/CMakeLists.txt $EXT_NAME/CMakeLists.txt
ln -s ../../src/MEGAShellExtNautilus/data $EXT_NAME/data
ln -s ../MEGAsync/MEGAsync/debian.copyright $EXT_NAME/debian.copyright

export GZIP=-9
tar czfh $EXT_NAME.tar.gz --exclude Makefile --exclude '*.o' $EXT_NAME
rm -rf $EXT_NAME

# delete any previous archive
rm -fr MEGAsync/MEGAShellExtNautilus/nautilus-megasync_*.tar.gz
# transform arch name, to satisfy Debian requirements
mv $EXT_NAME.tar.gz MEGAsync/MEGAShellExtNautilus/nautilus-megasync_$EXT_VERSION.tar.gz

#get md5sum and replace in PKGBUILD
MD5SUM=`md5sum MEGAsync/MEGAShellExtNautilus/nautilus-megasync_$EXT_VERSION.tar.gz | awk '{print $1}'`
sed "s/MD5SUM/$MD5SUM/g"  -i MEGAsync/MEGAShellExtNautilus/PKGBUILD



#
# Nemo
#

# make sure the source tree is in "clean" state
cd ../src/MEGAShellExtNemo/
make distclean 2> /dev/null || true
cd ../../build

#Get current version
export EXT_VERSION=`cat MEGAsync/MEGAShellExtNemo/debian.changelog | head -1 | sed -e 's#[^(]*(\([^)]*\))[^)]*#\1#g'`
export EXT_NAME=nemo-megasync-$EXT_VERSION
rm -rf $EXT_NAME.tar.gz
rm -rf $EXT_NAME

# delete previously generated files
rm -fr MEGAsync/MEGAShellExtNemo/nemo-megasync_*.dsc

# fix version number in template files and copy to appropriate directories
sed -e "s/EXT_VERSION/$EXT_VERSION/g" templates/MEGAShellExtNemo/nemo-megasync.spec > MEGAsync/MEGAShellExtNemo/nemo-megasync.spec
sed -e "s/EXT_VERSION/$EXT_VERSION/g" templates/MEGAShellExtNemo/nemo-megasync.dsc > MEGAsync/MEGAShellExtNemo/nemo-megasync_$EXT_VERSION.dsc
sed -e "s/EXT_VERSION/$EXT_VERSION/g" templates/MEGAShellExtNemo/PKGBUILD > MEGAsync/MEGAShellExtNemo/PKGBUILD

for dist in xUbuntu_{1,2}{0,1,2,3,4,5,6,7,8,9}.{04,10} Debian_{{7,8,9,10,11,12}.0,testing}; do
if [ -f templates/MEGAShellExtNemo/nemo-megasync-$dist.dsc ]; then
	sed -e "s/EXT_VERSION/$EXT_VERSION/g" templates/MEGAShellExtNemo/nemo-megasync-$dist.dsc > MEGAsync/MEGAShellExtNemo/nemo-megasync-$dist.dsc
else
	sed -e "s/EXT_VERSION/$EXT_VERSION/g" templates/MEGAShellExtNemo/nemo-megasync.dsc > MEGAsync/MEGAShellExtNemo/nemo-megasync-$dist.dsc
fi
done

# create archive
mkdir $EXT_NAME
ln -s ../MEGAsync/MEGAShellExtNemo/nemo-megasync.spec $EXT_NAME/nemo-megasync.spec
ln -s ../MEGAsync/MEGAShellExtNemo/debian.postinst $EXT_NAME/debian.postinst
ln -s ../../src/MEGAShellExtNemo/mega_ext_client.c $EXT_NAME/mega_ext_client.c
ln -s ../../src/MEGAShellExtNemo/mega_ext_client.h $EXT_NAME/mega_ext_client.h
ln -s ../../src/MEGAShellExtNemo/mega_ext_module.c $EXT_NAME/mega_ext_module.c
ln -s ../../src/MEGAShellExtNemo/mega_notify_client.h $EXT_NAME/mega_notify_client.h
ln -s ../../src/MEGAShellExtNemo/mega_notify_client.c $EXT_NAME/mega_notify_client.c
ln -s ../../src/MEGAShellExtNemo/MEGAShellExt.c $EXT_NAME/MEGAShellExt.c
ln -s ../../src/MEGAShellExtNemo/MEGAShellExt.h $EXT_NAME/MEGAShellExt.h
ln -s ../../src/MEGAShellExtNemo/MEGAShellExtNemo.pro $EXT_NAME/MEGAShellExtNemo.pro
ln -s ../../src/MEGAShellExtNemo/data $EXT_NAME/data
ln -s ../MEGAsync/MEGAsync/debian.copyright $EXT_NAME/debian.copyright

export GZIP=-9
tar czfh $EXT_NAME.tar.gz --exclude Makefile --exclude '*.o' $EXT_NAME
rm -rf $EXT_NAME

# delete any previous archive
rm -fr MEGAsync/MEGAShellExtNemo/nemo-megasync_*.tar.gz
# transform arch name, to satisfy Debian requirements
mv $EXT_NAME.tar.gz MEGAsync/MEGAShellExtNemo/nemo-megasync_$EXT_VERSION.tar.gz

#get md5sum and replace in PKGBUILD
MD5SUM=`md5sum MEGAsync/MEGAShellExtNemo/nemo-megasync_$EXT_VERSION.tar.gz | awk '{print $1}'`
sed "s/MD5SUM/$MD5SUM/g"  -i MEGAsync/MEGAShellExtNemo/PKGBUILD


#
# Thunar
#

# make sure the source tree is in "clean" state
cd ../src/MEGAShellExtThunar/
make distclean 2> /dev/null || true
cd ../../build

#Get current version
export EXT_VERSION=`cat MEGAsync/MEGAShellExtThunar/debian.changelog | head -1 | sed -e 's#[^(]*(\([^)]*\))[^)]*#\1#g'`
export EXT_NAME=thunar-megasync-$EXT_VERSION
rm -rf $EXT_NAME.tar.gz
rm -rf $EXT_NAME

# delete previously generated files
rm -fr MEGAsync/MEGAShellExtThunar/thunar-megasync_*.dsc

# fix version number in template files and copy to appropriate directories
sed -e "s/EXT_VERSION/$EXT_VERSION/g" templates/MEGAShellExtThunar/thunar-megasync.spec > MEGAsync/MEGAShellExtThunar/thunar-megasync.spec
sed -e "s/EXT_VERSION/$EXT_VERSION/g" templates/MEGAShellExtThunar/PKGBUILD > MEGAsync/MEGAShellExtThunar/PKGBUILD

for dist in xUbuntu_{1,2}{0,1,2,3,4,5,6,7,8,9}.{04,10} Debian_{{7,8,9,10,11,12}.0,testing}; do
if [ -f templates/MEGAShellExtThunar/thunar-megasync-$dist.dsc ]; then
	sed -e "s/EXT_VERSION/$EXT_VERSION/g" templates/MEGAShellExtThunar/thunar-megasync-$dist.dsc > MEGAsync/MEGAShellExtThunar/thunar-megasync-$dist.dsc
else
	sed -e "s/EXT_VERSION/$EXT_VERSION/g" templates/MEGAShellExtThunar/thunar-megasync.dsc > MEGAsync/MEGAShellExtThunar/thunar-megasync-$dist.dsc
fi
done


# create archive
mkdir $EXT_NAME
ln -s ../MEGAsync/MEGAShellExtThunar/thunar-megasync.spec $EXT_NAME/thunar-megasync.spec
ln -s ../../src/MEGAShellExtThunar/mega_ext_client.c $EXT_NAME/mega_ext_client.c
ln -s ../../src/MEGAShellExtThunar/mega_ext_client.h $EXT_NAME/mega_ext_client.h
ln -s ../../src/MEGAShellExtThunar/MEGAShellExt.c $EXT_NAME/MEGAShellExt.c
ln -s ../../src/MEGAShellExtThunar/MEGAShellExt.h $EXT_NAME/MEGAShellExt.h
ln -s ../../src/MEGAShellExtThunar/MEGAShellExtThunar.pro $EXT_NAME/MEGAShellExtThunar.pro
ln -s ../MEGAsync/MEGAsync/debian.copyright $EXT_NAME/debian.copyright

export GZIP=-9
tar czfh $EXT_NAME.tar.gz --exclude Makefile --exclude '*.o' $EXT_NAME
rm -rf $EXT_NAME

# delete any previous archive
rm -fr MEGAsync/MEGAShellExtThunar/thunar-megasync_*.tar.gz
# transform arch name, to satisfy Debian requirements
mv $EXT_NAME.tar.gz MEGAsync/MEGAShellExtThunar/thunar-megasync_$EXT_VERSION.tar.gz

#get md5sum and replace in PKGBUILD
MD5SUM=`md5sum MEGAsync/MEGAShellExtThunar/thunar-megasync_$EXT_VERSION.tar.gz | awk '{print $1}'`
sed "s/MD5SUM/$MD5SUM/g"  -i MEGAsync/MEGAShellExtThunar/PKGBUILD




#
# Dolphin
#

# make sure the source tree is in "clean" state
cd ../src/MEGAShellExtDolphin/
make distclean 2> /dev/null || true
cd ../../build

#Get current version
export EXT_VERSION=`cat MEGAsync/MEGAShellExtDolphin/debian.changelog | head -1 | sed -e 's#[^(]*(\([^)]*\))[^)]*#\1#g'`
export EXT_NAME=dolphin-megasync-$EXT_VERSION
rm -rf $EXT_NAME.tar.gz
rm -rf $EXT_NAME

# delete previously generated files
rm -fr MEGAsync/MEGAShellExtDolphin/dolphin-megasync_*.dsc

# fix version number in template files and copy to appropriate directories
sed -e "s/EXT_VERSION/$EXT_VERSION/g" templates/MEGAShellExtDolphin/dolphin-megasync.spec > MEGAsync/MEGAShellExtDolphin/dolphin-megasync.spec
#sed -e "s/EXT_VERSION/$EXT_VERSION/g" templates/MEGAShellExtDolphin/dolphin-megasync.dsc > MEGAsync/MEGAShellExtDolphin/dolphin-megasync_$EXT_VERSION.dsc

for dist in xUbuntu_{1,2}{0,1,2,3,4,5,6,7,8,9}.{04,10} Debian_{{7,8,9,10,11,12}.0,testing}; do
if [ -f templates/MEGAShellExtDolphin/dolphin-megasync-$dist.dsc ]; then
	sed -e "s/EXT_VERSION/$EXT_VERSION/g" templates/MEGAShellExtDolphin/dolphin-megasync-$dist.dsc > MEGAsync/MEGAShellExtDolphin/dolphin-megasync-$dist.dsc
else
	sed -e "s/EXT_VERSION/$EXT_VERSION/g" templates/MEGAShellExtDolphin/dolphin-megasync.dsc > MEGAsync/MEGAShellExtDolphin/dolphin-megasync-$dist.dsc
fi
done



sed -e "s/EXT_VERSION/$EXT_VERSION/g" templates/MEGAShellExtDolphin/PKGBUILD > MEGAsync/MEGAShellExtDolphin/PKGBUILD

# create archive
mkdir $EXT_NAME
ln -s ../MEGAsync/MEGAShellExtDolphin/dolphin-megasync.spec $EXT_NAME/dolphin-megasync.spec
ln -s ../../src/MEGAShellExtDolphin/megasync-plugin.cpp $EXT_NAME/megasync-plugin.cpp
ln -s ../../src/MEGAShellExtDolphin/megasync-plugin-overlay.cpp $EXT_NAME/megasync-plugin-overlay.cpp
ln -s ../../src/MEGAShellExtDolphin/megasync-plugin-overlay.json $EXT_NAME/megasync-plugin-overlay.json
ln -s ../../src/MEGAShellExtDolphin/data $EXT_NAME/data
ln -s ../../src/MEGAShellExtDolphin/CMakeLists.txt $EXT_NAME/CMakeLists.txt
ln -s ../../src/MEGAShellExtDolphin/megasync-plugin.h $EXT_NAME/megasync-plugin.h
ln -s ../../src/MEGAShellExtDolphin/megasync-plugin.desktop $EXT_NAME/megasync-plugin.desktop
ln -s ../../src/MEGAShellExtDolphin/MEGAShellExtDolphin.pro $EXT_NAME/MEGAShellExtDolphin.pro
ln -s ../MEGAsync/MEGAsync/debian.copyright $EXT_NAME/debian.copyright

export GZIP=-9
tar czfh $EXT_NAME.tar.gz --exclude Makefile --exclude '*.o' $EXT_NAME
rm -rf $EXT_NAME

# delete any previous archive
rm -fr MEGAsync/MEGAShellExtDolphin/dolphin-megasync_*.tar.gz
# transform arch name, to satisfy Debian requirements
mv $EXT_NAME.tar.gz MEGAsync/MEGAShellExtDolphin/dolphin-megasync_$EXT_VERSION.tar.gz

#get md5sum and replace in PKGBUILD
MD5SUM=`md5sum MEGAsync/MEGAShellExtDolphin/dolphin-megasync_$EXT_VERSION.tar.gz | awk '{print $1}'`
sed "s/MD5SUM/$MD5SUM/g"  -i MEGAsync/MEGAShellExtDolphin/PKGBUILD
