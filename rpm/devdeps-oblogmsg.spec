Name: devdeps-oblogmsg
Version: %(echo $VERSION)
Release: %(echo $RELEASE)%{?dist}

# if you want use the parameter of rpm_create on build time,
# uncomment below
Summary: oblogmsg
Group: Development/Tools
License: Commercial
Url: oceanbase.com

%define _prefix /usr/local/oceanbase/deps/devel
%define __strip /bin/true
%define __os_install_post %{nil}
%define debug_package %{nil}

# uncomment below, if your building depend on other packages

#BuildRequires: package_name = 1.0.0

# uncomment below, if depend on other packages

#Requires: package_name = 1.0.0

%description
# if you want publish current svn URL or Revision use these macros
Lua static library for oceanbase

#%debug_package
# support debuginfo package, to reduce runtime package size

# prepare your files
%install

mkdir -p $RPM_BUILD_ROOT/%{_prefix}
mkdir -p $RPM_BUILD_ROOT/%{_prefix}/include/oblogmsg
mkdir -p $RPM_BUILD_ROOT/%{_prefix}/lib
cd $OLDPWD/../
sh build.sh --init release
cd build_release
make -j8
cd ..
cp build_release/src/liboblogmsg.a $RPM_BUILD_ROOT/%{_prefix}/lib/
cp build_release/src/liboblogmsg.so $RPM_BUILD_ROOT/%{_prefix}/lib/
cp include/*.h $RPM_BUILD_ROOT/%{_prefix}/include/oblogmsg/

# package infomation
%files
# set file attribute here
%defattr(-,root,root)
# need not list every file here, keep it as this
%{_prefix}
## create an empy dir

# %dir %{_prefix}/var/log

## need bakup old config file, so indicate here

# %config %{_prefix}/etc/sample.conf

## or need keep old config file, so indicate with "noreplace"

# %config(noreplace) %{_prefix}/etc/sample.conf

## indicate the dir for crontab

# %attr(644,root,root) %{_crondir}/*

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig

%changelog
* Tue Sep 6 2022 wenxignsen.wxs
- for ob 4.0 opensource
(base)
