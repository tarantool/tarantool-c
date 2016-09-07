Name: tarantool-c
Version: 1.0.1
Release: 1%{?dist}
Summary: Tarantool C connector
Group: Development/Languages
License: BSD
URL: https://github.com/tarantool/tarantool-c
Source0: tarantool-c-%{version}.tar.gz
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
%setup -q -n %{name}-%{version}

%build
cmake . -DCMAKE_INSTALL_LIBDIR='%{_libdir}' -DCMAKE_INSTALL_INCLUDEDIR='%{_includedir}' -DCMAKE_BUILD_TYPE='RelWithDebInfo'
make %{?_smp_mflags}

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
* Tue Jun 9 2015 Eugine Blikh <bigbes@gmail.com> 1.0.1-1
- Couple of fixes for tarantool-c library
* Tue Jun 9 2015 Eugine Blikh <bigbes@gmail.com> 1.0.0-1
- Initial version of the RPM spec
