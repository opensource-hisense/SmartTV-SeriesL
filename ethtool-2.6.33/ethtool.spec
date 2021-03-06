Name		: ethtool
Version		: 2.6.33
Release		: 1
Group		: Utilities

Summary		: A tool for setting ethernet parameters

License		: GPL
URL		: http://sourceforge.net/projects/gkernel/

Buildroot	: %{_tmppath}/%{name}-%{version}
Source		: %{name}-%{version}.tar.gz


%description
Ethtool is a small utility to get and set values from your your ethernet 
controllers.  Not all ethernet drivers support ethtool, but it is getting 
better.  If your ethernet driver doesn't support it, ask the maintainer to 
write support - it's not hard!


%prep
%setup -q


%build
CFLAGS="${RPM_OPT_FLAGS}" ./configure --prefix=/usr --mandir=%{_mandir}
make


%install
make install DESTDIR=${RPM_BUILD_ROOT}


%files
%defattr(-,root,root)
/usr/sbin/ethtool
%{_mandir}/man8/ethtool.8*
%doc AUTHORS COPYING INSTALL NEWS README ChangeLog


%changelog
