Name:		megasync
Version:	MEGASYNC_VERSION
Release:	%(cat MEGA_BUILD_ID || echo "1").1
Summary:	Get more control over your data
License:	Freeware
Group:		Applications/Others
Url:		https://mega.nz
Source0:	megasync_%{version}.tar.gz
Vendor:		MEGA Limited
Packager:	MEGA Linux Team <linux@mega.co.nz>

BuildRequires: autoconf, autoconf-archive, automake, libtool, gcc-c++, libicu-devel
BuildRequires: hicolor-icon-theme, zip, unzip, nasm, cmake, perl

#OpenSUSE
%if 0%{?suse_version} || 0%{?sle_version}

    BuildRequires: libopenssl-devel, sqlite3-devel
    BuildRequires: libbz2-devel, zlib-devel
    BuildRequires: lsb-release, wget

    # disabling post-build-checks that ocassionally prevent opensuse rpms from being generated
    # plus it speeds up building process
    #!BuildIgnore: post-build-checks

    %if 0%{?sle_version} >= 150000
        BuildRequires: libcurl4
    %endif

    %if 0%{?suse_version} > 1500 || 0%{?sle_version} >= 150400
        BuildRequires: systemd-devel
    %else
        BuildRequires: libudev-devel
    %endif

    %if 0%{?suse_version} > 1500
        BuildRequires: pkgconf-pkg-config
    %else
        BuildRequires: pkg-config
        BuildRequires: gcc13 gcc13-c++
        BuildRequires: python311
    %endif

    %if 0%{?suse_version} > 1500 || 0%{?sle_version} >= 150300 || (0%{?is_opensuse} && 0%{?sle_version} >= 150000)
        BuildRequires: c-ares-devel
    %else
        BuildRequires: libcares-devel
    %endif

    %if !( ( "%{_target_cpu}" == "i586" && ( 0%{?sle_version} == 120200 || 0%{?sle_version} == 120300) ) || 0%{?suse_version} == 1230 )
        BuildRequires: libraw-devel
    %endif
        BuildRequires: update-desktop-files

    %if 0%{?sle_version} >= 120200 || 0%{?suse_version} > 1320
        BuildRequires: libqt5-qtbase-devel, libqt5-linguist-devel, libqt5-qtsvg-devel, libqt5-qtx11extras-devel, libqt5-qtdeclarative-devel
        Requires: libQt5Core5 libqt5-qtquickcontrols libqt5-qtquickcontrols2
    %else
        BuildRequires: libqt4-devel, qt-devel
    %endif

    %if 0%{?suse_version} <= 1320
        BuildRequires: libcryptopp-devel
    %endif

%else

    %if 0%{?rhel_version} == 0 && 0%{?centos_version} < 800
        #if !RHEL
        BuildRequires: LibRaw-devel
    %endif

%endif

%if 0%{?is_opensuse} && (0%{?sle_version} && 0%{?sle_version} <= 120300)
    BuildRequires: gcc5, gcc5-c++
%endif

#Fedora specific
%if 0%{?fedora}
    BuildRequires: openssl-devel, sqlite-devel, c-ares-devel
    BuildRequires: desktop-file-utils
    BuildRequires: bzip2-devel
    BuildRequires: systemd-devel
    BuildRequires: lsb_release, pkgconf-pkg-config

    %if 0%{?fedora_version} < 33
        BuildRequires: cryptopp-devel
        Requires: cryptopp >= 5.6.5
    %endif

    %if 0%{?fedora_version}==25
        BuildRequires: lz4-libs
    %endif

    %if 0%{?fedora_version} >= 31
        BuildRequires: fonts-filesystem
    %else
        BuildRequires: fontpackages-filesystem
    %endif

    %if 0%{?fedora_version} >= 40
        BuildRequires: wget2, wget2-wget, zlib-ng-compat
    %else
        BuildRequires: wget, zlib-devel
    %endif

    # allowing for rpaths (taken as invalid, as if they were not absolute paths when they are)
    %if 0%{?fedora_version} >= 35
        %define __brp_check_rpaths QA_RPATHS=0x0002 /usr/lib/rpm/check-rpaths
    %endif

    %if 0%{?fedora_version} >= 23
        BuildRequires: qt5-qtbase-devel qt5-qttools-devel, qt5-qtsvg-devel, qt5-qtx11extras-devel, qt5-qtdeclarative-devel
        Requires: qt5-qtbase >= 5.6, qt5-qtsvg, qt5-qtdeclarative, qqc2-desktop-style, qt5-qtquickcontrols, qt5-qtquickcontrols2
        BuildRequires: terminus-fonts
    %else
        BuildRequires: qt, qt-x11, qt-devel
        BuildRequires: terminus-fonts
    %endif
%endif

#centos/scientific linux
%if 0%{?centos_version} || 0%{?scientificlinux_version}
    BuildRequires: openssl-devel, sqlite-devel, c-ares-devel, bzip2-devel
    BuildRequires: desktop-file-utils
    BuildRequires: systemd-devel
    Requires: qt5-qtdeclarative

    %if 0%{?centos_version} >= 800
        BuildRequires: bzip2-devel
        BuildRequires: qt5-qtbase-devel qt5-qttools-devel, qt5-qtsvg-devel, qt5-qtx11extras-devel, qt5-qtdeclarative-devel
    %else
        BuildRequires: qt-mega, mesa-libGL-devel
        Requires: freetype >= 2.8
    %endif
%endif

#red hat
%if 0%{?rhel_version}
    BuildRequires: openssl-devel, sqlite-devel, desktop-file-utils
    %if 0%{?rhel_version} < 800
        BuildRequires: qt, qt-x11, qt-devel
    %else
        BuildRequires: qt5-qtbase-devel qt5-qttools-devel, qt5-qtsvg-devel, qt5-qtx11extras-devel, qt5-qtdeclarative-devel
        BuildRequires: bzip2-devel
    %endif
%endif


### Specific buildable dependencies ###

%if 0%{?fedora_version}==21 || 0%{?fedora_version}==22 || 0%{?fedora_version}>=25 || !(0%{?sle_version} < 120300)
    BuildRequires: libzen-devel, libmediainfo-devel
%endif

%description
- Sync your entire MEGA Cloud or selected folders with your computer so your MEGA stays up to date with the changes you make to your data on your computer and vice versa.

- Back up your computer with MEGA to automatically copy data to MEGA in real time and eliminate the risk of accidental data loss.

- Easily add, sort, search for, prioritise, pause, and cancel your uploads and downloads using our transfer manager.

%prep
%setup -q

mega_build_id=`echo %{release} | sed "s/\.[^.]*$//" | sed "s/[^.]*\.//" | sed "s/[^0-9]//g"`
sed -i -E "s/VER_PRODUCTVERSION_STR([[:space:]]+)\"(([0-9][0-9]*\.){3})(.*)\"/VER_PRODUCTVERSION_STR\1\"\2${mega_build_id}\"/g" src/MEGASync/control/Version.h
sed -i -E "s/VER_BUILD_ID([[:space:]]+)([0-9]*)/VER_BUILD_ID\1${mega_build_id}/g" src/MEGASync/control/Version.h

%if ( 0%{?fedora_version} && 0%{?fedora_version}<=38 ) || ( 0%{?centos_version} == 600 ) || ( 0%{?centos_version} == 800 ) || ( 0%{?sle_version} && 0%{?sle_version} < 150500 )
    %define extradefines -DMEGASYNC_DEPRECATED_OS
%else
    %define extradefines %{nil}
%endif

%if 0%{?centos_version} && 0%{?centos_version} < 800
    %define qtinstall "-DDEPLOY_QT_LIBRARIES=ON"
    %define qtprefix "-DCMAKE_PREFIX_PATH=/opt/mega/"
%else
    %define qtinstall %{nil}
    %define qtprefix %{nil}
%endif

if [ -f /opt/vcpkg.tar.gz ]; then
    export VCPKG_DEFAULT_BINARY_CACHE=/opt/persistent/vcpkg_cache
    mkdir -p ${VCPKG_DEFAULT_BINARY_CACHE}
    tar xzf /opt/vcpkg.tar.gz
    vcpkg_root="-DVCPKG_ROOT=vcpkg"
fi

if [ -f /opt/cmake.tar.gz ]; then
    echo "8dc99be7ba94ad6e14256b049e396b40  /opt/cmake.tar.gz" | md5sum -c -
    tar xzf /opt/cmake.tar.gz
    ln -s cmake-*-Linux* cmake_inst
    export PATH="${PWD}/cmake_inst/bin:${PATH}"
fi

if [ -f /opt/mega/bin/qmake ]; then
    qtdefinitions="%{qtprefix} %{qtinstall}"
fi

if [ -n "%{extradefines}" ]; then
    export CXXFLAGS="%{extradefines} ${CXXFLAGS}"
fi

# OpenSuse Leap 15.x defaults to gcc7.
# Python>=10 needed for VCPKG pkgconf
%if 0%{?suse_version} && 0%{?suse_version} <= 1500
    export CC=gcc-13
    export CXX=g++-13
    mkdir python311
    ln -sf /usr/bin/python3.11 python311/python3
    export PATH=$PWD/python311:$PATH
%endif

cmake --version
cmake ${vcpkg_root} -DENABLE_DESKTOP_UPDATE_GEN=OFF -DENABLE_DESIGN_TOKENS_IMPORTER=OFF ${qtdefinitions} -DCMAKE_VERBOSE_MAKEFILE=ON -DCMAKE_BUILD_TYPE=RelWithDebInfo -S . -B %{_builddir}/build_dir

%build

%if 0%{?is_opensuse} && (0%{?sle_version} && 0%{?sle_version} <= 120300)
    # ln to gcc/g++ v5, instead of default 4.8
    mkdir userPath
    ln -sf /usr/bin/gcc-5 userPath/gcc
    ln -sf /usr/bin/g++-5 userPath/g++
    export PATH=`pwd`/userPath:$PATH
%endif

if [ -f /opt/cmake.tar.gz ]; then
    export PATH="${PWD}/cmake_inst/bin:${PATH}"
fi

cmake --build %{_builddir}/build_dir %{?_smp_mflags}

%install

if [ -f /opt/cmake.tar.gz ]; then
    export PATH="${PWD}/cmake_inst/bin:${PATH}"
fi

cmake --install %{_builddir}/build_dir --prefix %{buildroot}

%if 0%{?suse_version}
    %suse_update_desktop_file -n -i %{name} Network System
%else
    desktop-file-install \
        --add-category="Network" \
        --dir %{buildroot}%{_datadir}/applications \
    %{buildroot}%{_datadir}/applications/%{name}.desktop
%endif

mkdir -p  %{buildroot}/etc/sysctl.d/
echo "fs.inotify.max_user_watches = 524288" > %{buildroot}/etc/sysctl.d/99-megasync-inotify-limit.conf

mkdir -p  %{buildroot}/etc/udev/rules.d/
echo "SUBSYSTEM==\"block\", ATTRS{idDevtype}==\"partition\"" > %{buildroot}/etc/udev/rules.d/99-megasync-udev.rules

mkdir -p  %{buildroot}%{_docdir}/%{name}
lsb_release -ds > %{buildroot}%{_docdir}/%{name}/distro
lsb_release -rs > %{buildroot}%{_docdir}/%{name}/version

%post
%if 0%{?suse_version} >= 1140
    %desktop_database_post
    %icon_theme_cache_post
%else
    /bin/touch --no-create %{_datadir}/icons/hicolor &>/dev/null || :
    /bin/touch --no-create %{_datadir}/icons/ubuntu-mono-dark &>/dev/null || :
%endif


## Configure repository ##

%if 0%{?fedora}

    YUM_FILE="/etc/yum.repos.d/megasync.repo"
    cat > "$YUM_FILE" << DATA
[MEGAsync]
name=MEGAsync
baseurl=https://mega.nz/linux/repo/Fedora_\$releasever/
gpgkey=https://mega.nz/linux/repo/Fedora_\$releasever/repodata/repomd.xml.key
gpgcheck=1
enabled=1
DATA

%endif

%if 0%{?rhel_version} || 0%{?centos_version} || 0%{?scientificlinux_version}

    %if 0%{?rhel_version} == 800
        %define reponame RHEL_8
    %endif

    %if 0%{?rhel_version} == 700
        %define reponame RHEL_7
    %endif

    %if 0%{?scientificlinux_version} == 700
        %define reponame ScientificLinux_7
    %endif

    %if 0%{?centos_version} == 700
        %define reponame CentOS_7
    %endif

    %if 0%{?centos_version} == 800
        %define reponame CentOS_8
    %endif

    YUM_FILE="/etc/yum.repos.d/megasync.repo"
    cat > "$YUM_FILE" << DATA
[MEGAsync]
name=MEGAsync
baseurl=https://mega.nz/linux/repo/%{reponame}/
gpgkey=https://mega.nz/linux/repo/%{reponame}/repodata/repomd.xml.key
gpgcheck=1
enabled=1
DATA

%endif

%if 0%{?sle_version} || 0%{?suse_version}
    %if 0%{?sle_version} == 120300
        %define reponame openSUSE_Leap_42.3
    %endif

    %if 0%{?sle_version} == 120200
        %define reponame openSUSE_Leap_42.2
    %endif

    %if 0%{?sle_version} == 120100
        %define reponame openSUSE_Leap_42.1
    %endif

    %if 0%{?sle_version} == 150000 || 0%{?sle_version} == 150100 || 0%{?sle_version} == 150200
        %define reponame openSUSE_Leap_15.0
    %endif

    %if 0%{?sle_version} == 150300
        %define reponame openSUSE_Leap_15.3
    %endif

    %if 0%{?sle_version} == 150400
        %define reponame openSUSE_Leap_15.4
    %endif

    %if 0%{?sle_version} == 150500
        %define reponame openSUSE_Leap_15.5
    %endif

    %if 0%{?sle_version} == 150600
        %define reponame openSUSE_Leap_15.6
    %endif

    %if 0%{?sle_version} == 0 && 0%{?suse_version} >= 1550
        %define reponame openSUSE_Tumbleweed
    %endif

    %if 0%{?suse_version} == 1320
        %define reponame openSUSE_13.2
    %endif

    %if 0%{?suse_version} == 1310
        %define reponame openSUSE_13.1
    %endif

    %if 0%{?suse_version} == 1230
        %define reponame openSUSE_12.3
    %endif
    %if 0%{?suse_version} == 1220
        %define reponame openSUSE_12.2
    %endif

    if [ -d "/etc/zypp/repos.d/" ]; then
        ZYPP_FILE="/etc/zypp/repos.d/megasync.repo"
        cat > "$ZYPP_FILE" << DATA
[MEGAsync]
name=MEGAsync
type=rpm-md
baseurl=https://mega.nz/linux/repo/%{reponame}/
gpgcheck=1
autorefresh=1
gpgkey=https://mega.nz/linux/repo/%{reponame}/repodata/repomd.xml.key
enabled=1
DATA
    fi

%endif

### include public signing key #####
# Install new key if it's not present
# Notice, for openSuse, postinst is checked (and therefore executed) when creating the rpm
# we need to ensure no command results in fail (returns !=0)

# Remove old key if present.
if (rpm -q gpg-pubkey-7f068e5d-563dc081 &> /dev/null); then
    mv /var/lib/rpm/.rpm.lock /var/lib/rpm/.rpm.lock_moved || : #to allow key management.
    %if 0%{?suse_version}
        #Key management would fail due to lock in /var/lib/rpm/Packages. We create a copy
        cp /var/lib/rpm/Packages{,_moved}
        mv /var/lib/rpm/Packages{_moved,}
    %endif
    rpm -e gpg-pubkey-7f068e5d-563dc081
    mv /var/lib/rpm/.rpm.lock_moved /var/lib/rpm/.rpm.lock || : #take it back
fi

rpm -q gpg-pubkey-7094a482-61ded129 > /dev/null 2>&1 || KEY_NOT_FOUND=1

if [ ! -z "$KEY_NOT_FOUND" ]; then

    KEYFILE=$(mktemp /tmp/megasync.XXXXXX || :)
    if [ -n "$KEYFILE" ]; then

            cat > "$KEYFILE" <<KEY || :
-----BEGIN PGP PUBLIC KEY BLOCK-----

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
sAdVa6xer/c=
=F8S0
-----END PGP PUBLIC KEY BLOCK-----
KEY

        mv /var/lib/rpm/.rpm.lock /var/lib/rpm/.rpm.lock_moved || : #to allow rpm import within postinst

        %if 0%{?suse_version}
            #Key import would hang and fail due to lock in /var/lib/rpm/Packages. We create a copy
            cp /var/lib/rpm/Packages{,_moved}
            mv /var/lib/rpm/Packages{_moved,}
        %endif

        rpm --import "$KEYFILE" 2>&1 || FAILED_IMPORT=1

        mv /var/lib/rpm/.rpm.lock_moved /var/lib/rpm/.rpm.lock || : #take it back

        rm $KEYFILE || :
    fi
fi

sysctl -p /etc/sysctl.d/99-megasync-inotify-limit.conf

### END of POSTINST


%preun
if [ "$1" == "1" ]; then
    killall -s SIGUSR1 megasync 2> /dev/null || true
else
    killall megasync 2> /dev/null || true
    username=$SUDO_USER 2> /dev/null || true
    # Check if the variable is empty (e.g. if the script is not executed with sudo)
    [ -z "$username" ] && username=$(whoami) 2> /dev/null || true
    su -c 'timeout 1 megasync --send-uninstall-event' $username 2> /dev/null || true
fi
sleep 2


%postun
%if 0%{?suse_version} >= 1140
    %desktop_database_postun
    %icon_theme_cache_postun
%else
    if [ $1 -eq 0 ] ; then
        /bin/touch --no-create %{_datadir}/icons/hicolor &>/dev/null || :
        /bin/touch --no-create %{_datadir}/icons/ubuntu-mono-dark &>/dev/null || :
        /usr/bin/gtk-update-icon-cache %{_datadir}/icons/* &>/dev/null || :
    fi
%endif


%posttrans
%if 0%{?fedora} || 0%{?rhel_version} || 0%{?centos_version} || 0%{?scientificlinux_version}
    /bin/touch --no-create %{_datadir}/icons/hicolor &>/dev/null || :
    /bin/touch --no-create %{_datadir}/icons/ubuntu-mono-dark &>/dev/null || :
    /usr/bin/gtk-update-icon-cache %{_datadir}/icons/* &>/dev/null || :
%endif
# to restore dormant MEGAsync upon updates
killall -s SIGUSR2 megasync 2> /dev/null || true


%clean
%{?buildroot:%__rm -rf "%{buildroot}"}


%files
%defattr(-,root,root)
%{_bindir}/%{name}
%{_datadir}/applications/megasync.desktop
%{_datadir}/icons/hicolor/*/*/mega.png
%{_datadir}/icons/hicolor/*/*/*
%{_datadir}/icons/*/*/*/*
%{_docdir}/megasync
%{_docdir}/megasync/*
/etc/sysctl.d/99-megasync-inotify-limit.conf
/etc/udev/rules.d/99-megasync-udev.rules
/opt/*

%changelog
