%global build_version %(git describe --long | sed "s/[0-9]*\.[0-9]*\.[0-9]*-//" | sed "s/-[a-z 0-9]*//")

Name: tarantool-c
Version: 1.0.0
Release: %{build_version}
Summary: Tarantool C connector
Group: Development/Languages
License: BSD
URL: https://github.com/tarantool/tarantool-c
Source0: https://github.com/tarantool/tarantool-c/archive/%{version}.tar.gz
# BuildRequires: cmake
# Strange bug.
# Fix according to http://www.jethrocarr.com/2012/05/23/bad-packaging-habits/
BuildRequires: cmake
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
Vendor: tarantool.org
Group: Applications/Databases
%description
C client libraries for Tarantool

%package devel
Summary: Development files for C libtnt
Requires: tarantool-c%{?_isa} = %{version}-%{release}
%description devel
C client development headers for Tarantool

##################################################################

%prep
%setup -c -q %{name}-%{version}

%build
cmake . -DCMAKE_INSTALL_LIBDIR='%{_libdir}' -DCMAKE_INSTALL_INCLUDEDIR='%{_includedir}' -DCMAKE_BUILD_TYPE='Release'
make

%install
make DESTDIR=%{buildroot} install

%files
"%{_libdir}/libtarantool.a"
"%{_libdir}/libtarantool.so"
"%{_libdir}/libtarantool.so.*"

%files devel
%dir "%{_includedir}/tarantool"
"%{_includedir}/tarantool/*.h"

%changelog
* Tue Jun 9 2015 Eugine Blikh <bigbes@gmail.com> 1.0.0-1
- Initial version of the RPM spec
