#
# spec file for package dragoman-kcm (openSUSE and Fedora targets on OBS)
#
# SPDX-FileCopyrightText: 2026 Blagovest Petrov <blagovest@petrovs.info>
# SPDX-FileCopyrightText: 2026 Vute Tech Ltd. <https://vute.tech>
# SPDX-License-Identifier: GPL-3.0-or-later
#
# @VERSION@ is stamped by packaging/obs/prepare.sh from the release tag; the
# release tarball carries it in .tarball-version as well, for CMake.

Name:           dragoman-kcm
Version:        @VERSION@
Release:        0
Summary:        System Settings module for the Dragomand translation daemon
License:        GPL-3.0-or-later
URL:            https://dragomand.l10n-bg.dev
Source0:        %{name}-%{version}.tar.gz

# The dragomand daemon it configures exists for these two only.
ExclusiveArch:  x86_64 aarch64

BuildRequires:  cmake >= 3.24
BuildRequires:  gcc-c++
BuildRequires:  gettext
BuildRequires:  cmake(DragomanQt)
BuildRequires:  cmake(KF6CoreAddons) >= 6.13
BuildRequires:  cmake(KF6I18n) >= 6.13
BuildRequires:  cmake(KF6KCMUtils) >= 6.13
BuildRequires:  cmake(KF6KIO) >= 6.13
BuildRequires:  cmake(KF6Service) >= 6.13
BuildRequires:  cmake(Qt6Core) >= 6.8
BuildRequires:  cmake(Qt6DBus) >= 6.8
BuildRequires:  cmake(Qt6Gui) >= 6.8
BuildRequires:  cmake(Qt6Qml) >= 6.8
BuildRequires:  cmake(Qt6Quick) >= 6.8
BuildRequires:  cmake(Qt6Test) >= 6.8
%if 0%{?fedora}
BuildRequires:  extra-cmake-modules >= 6.13
BuildRequires:  ninja-build
BuildRequires:  dbus-daemon
BuildRequires:  desktop-file-utils
# The page's QML imports, loaded at run time.
Requires:       qt6qml(org.kde.kcmutils)
Requires:       qt6qml(org.kde.kirigami)
Requires:       qt6qml(org.kde.kirigamiaddons.formcard)
%else
BuildRequires:  kf6-extra-cmake-modules >= 6.13
BuildRequires:  ninja
BuildRequires:  dbus-1
# The page's QML imports, loaded at run time.
Requires:       kf6-kcmutils-imports
Requires:       kf6-kirigami-imports
Requires:       kirigami-addons6
%endif
Requires:       dragomand

%description
The Offline Translation page of System Settings, for the Dragomand
daemon: its memory and network settings, the installed language pairs
with their quality, removal of downloaded pairs and model updates, and
the daemon's status.

%prep
%setup -q

%build
cmake -S . -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DCMAKE_INSTALL_PREFIX=%{_prefix} \
    -DKDE_INSTALL_USE_QT_SYS_PATHS=ON \
    -DBUILD_TESTING=ON
cmake --build build %{?_smp_mflags}

%check
ctest --test-dir build --output-on-failure

%install
DESTDIR=%{buildroot} cmake --install build
%find_lang kcm_dragomand

%files -f kcm_dragomand.lang
%license LICENSES/GPL-3.0-or-later.txt
%doc README.md
# System Settings loads modules from here; own the directories, which no
# package does on every distribution.
%dir %{_libdir}/qt6/plugins/plasma
%dir %{_libdir}/qt6/plugins/plasma/kcms
%dir %{_libdir}/qt6/plugins/plasma/kcms/systemsettings
%{_libdir}/qt6/plugins/plasma/kcms/systemsettings/kcm_dragomand.so
%{_datadir}/applications/kcm_dragomand.desktop

%changelog
* @RPM_DATE@ Blagovest Petrov <blagovest@petrovs.info> - @VERSION@
- Release @VERSION@
