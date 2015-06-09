Name: tnt
Version: 1.0.0
Release: 1%{?dist}
Summary: Tarantool C connector
Group: Development/Languages
License: BSD
URL: https://github.com/tarantool/tarantool-c
Source0: https://github.com/tarantool/tarantool-c/archive/1.0.0.tar.gz
# BuildRequires: cmake
# Strange bug.
# Fix according to http://www.jethrocarr.com/2012/05/23/bad-packaging-habits/
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
Vendor: tarantool.org
Group: Applications/Databases
%description
C client libraries for Tarantool/Box

%package devel
Summary: Development files for C libtnt
Requires: tnt%{?_isa} = 1.0.0-%{release}
%description devel
stupid shit

##################################################################

%prep
%setup -n tarantool-c-draft

%build
cmake . -DCMAKE_INSTALL_LIBDIR='%{_libdir}' -DCMAKE_INSTALL_INCLUDEDIR='%{_includedir}' -DCMAKE_BUILD_TYPE='Release'
make

%install
make DESTDIR=%{buildroot} install

%files
"%{_libdir}/libtarantool.a"
"%{_libdir}/libtarantool.so"
"%{_libdir}/libtarantool.so.*"
"%{_libdir}/libtarantoolnet.a"
"%{_libdir}/libtarantoolnet.so"
"%{_libdir}/libtarantoolnet.so.*"

%files devel
%dir "%{_includedir}/tarantool"
"%{_includedir}/tarantool/*.h"

%changelog
* Tue Jun 9 2015 Eugine Blikh <bigbes@gmail.com> 1.0.0-1
- Initial version of the RPM spec
