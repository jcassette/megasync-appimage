Name:       dolphin-megasync
Version:    EXT_VERSION
Release:	%(cat MEGA_BUILD_ID || echo "1").1
Summary:	MEGA Desktop App plugin for Dolphin
License:	Freeware
Group:		Applications/Others
Url:		https://mega.nz
Source0:	dolphin-megasync_%{version}.tar.gz
Vendor:		MEGA Limited
Packager:	MEGA Linux Team <linux@mega.co.nz>

AutoReq: 0

#OpenSUSE
%if 0%{?suse_version} || 0%{?sle_version}
%if 0%{?sle_version} >= 120100 || 0%{?suse_version} < 1500
BuildRequires:  kdelibs4support extra-cmake-modules libQt5Core-devel libQt5Network-devel kio-devel
%global debug_package %{nil}
%endif

%if 0%{?sle_version} == 0 && 0%{?suse_version} >= 1500
BuildRequires:  kdelibs4support libQt5Core-devel libQt5Network-devel kio-devel kf6-extra-cmake-modules
%global debug_package %{nil}
%endif
%endif

#Fedora specific
%if 0%{?fedora}
BuildRequires: extra-cmake-modules, kf5-kdelibs4support, kf5-kio-devel
%endif

%if 0%{?rhel_version} || 0%{?scientificlinux_version}
BuildRequires: kdelibs-devel gcc-c++
%endif

%if 0%{?centos_version}
BuildRequires: extra-cmake-modules, kf5-kdelibs4support, kf5-kio-devel
%endif

Requires:       megasync >= 5.3.0

%description
- Easily see and track your sync statuses.

- Send files and folders to MEGA.

- Share your synced files and folders with anyone by creating links.

- View files in MEGA's browser (webclient).

%prep
%setup -q

%build
# Create a temporary file containing the list of files
EXTRA_FILES=%{buildroot}/ExtraFiles.list
touch %{EXTRA_FILES}

cmake3 -DCMAKE_INSTALL_PREFIX="`kf5-config --prefix`" $PWD || cmake -DCMAKE_INSTALL_PREFIX="`kf5-config --prefix`" $PWD
make
make install DESTDIR=%{buildroot}

echo %(kf5-config --path services | awk -NF ":" '{print $NF}')/megasync-plugin.desktop >> %{EXTRA_FILES}
echo %(kf5-config --path lib | awk -NF ":" '{print $1}')/qt5/plugins/megasyncplugin.so >> %{EXTRA_FILES}

if [ -d %{buildroot}/%(kf5-config --path lib | awk -NF ":" '{print $1}')/qt5/plugins/kf5/overlayicon ]; then
echo %(kf5-config --path lib | awk -NF ":" '{print $1}')/qt5/plugins/kf5/overlayicon/megasyncdolphinoverlayplugin.so >> %{EXTRA_FILES}
echo %(kf5-config --path lib | awk -NF ":" '{print $1}')/qt5/plugins/kf5/overlayicon >> %{EXTRA_FILES}
echo %(kf5-config --path lib | awk -NF ":" '{print $1}')/qt5/plugins/kf5 >> %{EXTRA_FILES}
echo '%{_datadir}/icons/hicolor/*/*/mega-*.png' >> %{EXTRA_FILES}
echo '%{_datadir}/icons/hicolor/*/*' >> %{EXTRA_FILES}
fi

%if 0%{?centos_version} || 0%{?rhel_version} || 0%{?scientificlinux_version}
#fix conflict with existing /usr/lib64 (pointing to /usr/lib)
if [ -d %{buildroot}/usr/lib ]; then
    rsync -av %{buildroot}/usr/lib/ %{buildroot}/usr/lib64/
    rm -rf %{buildroot}/usr/lib
fi
%endif

%clean
echo cleaning
%{?buildroot:%__rm -rf "%{buildroot}"}

%files -f %{EXTRA_FILES}
%defattr(-,root,root)

%changelog
