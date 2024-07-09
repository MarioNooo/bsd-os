#
# ifhp Spec File
#
%define name ifhp
%define version 3.3.9
%define release 1

Summary: ifhp print filter for hp, postscript, text, and other printers
Name: %{name}
Version: %{version}
Release: 1
Copyright: OpenSource
Group: System Environment/Daemons
Source: ftp://ftp.astart.com/pub/LPRng/LPRng/FILTERS/%{name}-%{version}.tgz
BuildRoot: /var/tmp/%{name}-%{version}-%{release}-root
URL: http://www.astart.com/LPRng.html
Vendor: Astart Technologies, San Diego, CA 92123 http://www.astart.com
Packager: Patrick Powell <papowell@astart.com>

%description

ifhp is a highly versatile print filter for BSD based print spoolers.
It can be configured to handle text,  PostScript, PJL, PCL, and raster
printers, supports conversion from one format to another, and can be used 
as a stand-alone print utility.  

It is the primary supported print filter for the LPRng print spooler.

%prep

%setup

%build
#perl -spi.bak -e 's/^MAN=.*/MAN=\@mandir\@/' man/Makefile.in
# configure
./configure --sysconfdir=/etc --prefix=/usr
make
cd $RPM_BUILD_DIR/%{name}-%{version}/HOWTO;
gzip -9 *.txt *.ps


%install

rm -rf $RPM_BUILD_ROOT

# rhl init script
mkdir -p $RPM_BUILD_ROOT/etc $RPM_BUILD_ROOT/usr

make prefix=$RPM_BUILD_ROOT/usr	\
     sysconfdir=$RPM_BUILD_ROOT/etc \
	 install

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%attr(644,root,root) %config(noreplace) /etc/ifhp.conf
%attr(644,root,root) /etc/ifhp.conf.sample
%attr(755,root,root)  /usr/libexec/filters/*
%doc BUGS CHANGES Commercial.license Copyright INSTALL
%doc LICENSE README* VERSION
%doc HOWTO/*.html HOWTO/*.gz HOWTO/*.info
/usr/man/*/*
# end of SPEC file
