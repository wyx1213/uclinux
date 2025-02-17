# -*- rpm-spec -*-

## rpmbuild options

# default is to build with video support & without truespeech support
%define		video		%{?_without_video:0}%{!?_without_video:1}
%define		truespeech	%{?_with_truespeech:1}%{!?_with_truespeech:0}

# Linphone requires an old osip version, sometimes (e.g. fc6)
# delivered as "compat-"
%define _without_old_osip	0

Name:           linphone
Version:        2.0.1
Release:        1%{?dist}
Summary:        Phone anywhere in the whole world by using the Internet

Group:          Applications/Communications
License:        GPL
URL:            http://www.linphone.org
Source0:        http://download.savannah.gnu.org/releases/linphone/stable/source/%{name}-%{version}.tar.gz
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
%ifarch %{ix86}
BuildArch:	i686
%endif

BuildRequires:  gnome-panel-devel libgnomeui-devel glib2-devel alsa-lib-devel
BuildRequires:  libosip2-devel speex-devel gettext desktop-file-utils
BuildRequires:	readline-devel ncurses-devel
BuildRequires:  intltool gettext-devel
%if %{video}
BuildRequires:	ffmpeg-devel SDL-devel
%endif

%description
Linphone is mostly sip compliant. It works successfully with these
implementations:
    * eStara softphone (commercial software for windows)
    * Pingtel phones (with DNS enabled and VLAN QOS support disabled).
    * Hotsip, a free of charge phone for Windows.
    * Vocal, an open source SIP stack from Vovida that includes a SIP proxy
        that works with linphone since version 0.7.1.
    * Siproxd is a free sip proxy being developped by Thomas Ries because he
        would like to have linphone working behind his firewall. Siproxd is
        simple to setup and works perfectly with linphone.
    * Partysip aims at being a generic and fully functionnal SIP proxy. Visit
        the web page for more details on its functionalities.

Linphone may work also with other sip phones, but this has not been tested yet.

%package devel
Summary:        Development libraries for linphone
Group:          Development/Libraries
Requires:       %{name} = %{version}-%{release}
Requires:	ortp-devel = 0.14.0
Requires:	glib2-devel

%description    devel
Libraries and headers required to develop software with linphone.

%package -n ortp
Summary:        A C library implementing the RTP protocol (rfc1889)
Group:          System Environment/Libraries
Version:        0.14.0

%description -n ortp
oRTP is a LGPL licensed C library implementing the RTP protocol (rfc1889). It
is available for most *nix clones (primilarly Linux and HP-UX), and Win32.

%package -n ortp-devel
Summary:        Development libraries for ortp
Group:          Development/Libraries
Version:        0.14.0
Requires:	ortp = 0.14.0

%description -n ortp-devel
oRTP is a LGPL licensed C library implementing the RTP protocol (rfc1889). It
is available for most *nix clones (primilarly Linux and HP-UX), and Win32.

This package contains header files and development libraries needed to
develop programs using the oRTP library.

%package -n mediastreamer2
Summary:        Audio/Video real-time streaming
Group:          Development/Libraries
Version:        2.2.0

%description -n mediastreamer2
Mediastreamer2 is a GPL licensed library to make audio and video
real-time streaming and processing. Written in pure C, it is based
upon the oRTP library.

%package -n mediastreamer2-devel
Summary:        Headers, libraries and docs for the mediastreamer2 library
Group:          Development/Libraries
Version:        2.2.0
Requires:	mediastreamer2 = 2.2.0
Requires:	ortp-devel = 0.14.0

%description -n mediastreamer2-devel
Mediastreamer2 is a GPL licensed library to make audio and video
real-time streaming and processing. Written in pure C, it is based
upon the ortp library.

This package contains header files and development libraries needed to
develop programs using the mediastreamer2 library.

%prep
%setup -q
#%patch -p 1 -b .pkgconfig
#%patch1 -p 1 -b .Werror
#%patch2 -p 1 -b .old

%build
%configure \
	--with-osip=/usr \
	--with-speex=/usr \
	--with-readline=/usr \
%if %{video}
	--enable-video \
	--with-ffmpeg=/usr \
	--with-sdl=/usr \
%endif
%if %{truespeech}
	--enable-truespeech \
%endif
%{?_without_old_osip: --with-osip-version=2.2.2} \
	--enable-ipv6
%__make %{?_smp_mflags}


%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT
install -p -m 0644 pixmaps/linphone2.png $RPM_BUILD_ROOT%{_datadir}/pixmaps
%find_lang %{name}
rm $RPM_BUILD_ROOT%{_datadir}/gnome/apps/Internet/linphone.desktop
desktop-file-install --vendor=fedora \
  --delete-original \
  --dir $RPM_BUILD_ROOT%{_datadir}/applications \
  --add-category X-Fedora \
  --add-category Telephony \
  --add-category GTK \
  $RPM_BUILD_ROOT%{_datadir}/applications/%{name}.desktop

%clean
rm -rf $RPM_BUILD_ROOT

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%post -n ortp -p /sbin/ldconfig

%postun -n ortp -p /sbin/ldconfig

%post -n mediastreamer2 -p /sbin/ldconfig

%postun -n mediastreamer2 -p /sbin/ldconfig

%files -f %{name}.lang
%defattr(-,root,root)
%doc AUTHORS ChangeLog COPYING NEWS README TODO
%{_bindir}/*
%{_libdir}/bonobo/servers/*.server
%{_libdir}/liblinphone.so.*
%exclude %{_libdir}/libortp*
%{_libexecdir}/*
%{_mandir}/*
%{_datadir}/applications/*%{name}.desktop
%{_datadir}/gnome/help/linphone
%{_datadir}/gnome-2.0/ui/*.xml
%{_datadir}/pixmaps/linphone
%{_datadir}/pixmaps/linphone2.png
%{_datadir}/sounds/linphone

%files devel
%defattr(-,root,root)
%{_includedir}/linphone
%{_libdir}/liblinphone.a
%{_libdir}/liblinphone.la
%{_libdir}/liblinphone.so
%{_libdir}/pkgconfig/linphone.pc

%files -n ortp
%defattr(-,root,root)
%doc oRTP/AUTHORS oRTP/ChangeLog oRTP/COPYING oRTP/NEWS oRTP/README oRTP/TODO
%{_libdir}/libortp.so.*
%exclude %{_libdir}/liblinphone*

%files -n ortp-devel
%defattr(-,root,root)
%{_includedir}/ortp
%{_libdir}/pkgconfig/ortp.pc
%{_libdir}/libortp.a
%{_libdir}/libortp.la
%{_libdir}/libortp.so
%{_datadir}/gtk-doc/html/ortp

%files -n mediastreamer2
%defattr(-,root,root)
%doc mediastreamer2/AUTHORS mediastreamer2/ChangeLog mediastreamer2/COPYING
%doc mediastreamer2/NEWS mediastreamer2/README
%{_libdir}/libmediastreamer.so.*
%{_libdir}/libquickstream.so.*

%files -n mediastreamer2-devel
%{_includedir}/mediastreamer2
%{_libdir}/pkgconfig/mediastreamer.pc
%{_libdir}/libmediastreamer.so
%{_libdir}/libmediastreamer.*a
%{_libdir}/libquickstream.so
%{_libdir}/libquickstream.*a

%changelog
* Wed Sep 28 2005 Francois-Xavier 'FiX' KOWALSKI <francois-xavier.kowalski@hp.com> - 1.2.0pre3
- Updated to latests Simon's work

* Fri May 27 2005 Ignacio Vazquez-Abrams <ivazquez@ivazquez.net> 1.0.1-3
- Fix multiple menu entry and missing icon (#158975)
- Clean up spec file

* Fri May  6 2005 Ignacio Vazquez-Abrams <ivazquez@ivazquez.net> 1.0.1-2
- Fix libosip2-devel BR

* Wed May  4 2005 Ignacio Vazquez-Abrams <ivazquez@ivazquez.net> 1.0.1-1
- Update to 1.0.1
- Port patches from devel

* Wed Mar 23 2005 Ignacio Vazquez-Abrams <ivazquez@ivazquez.net> 0.12.2-7
- pkgconfig and -devel fixes

* Wed Mar 23 2005 Ignacio Vazquez-Abrams <ivazquez@ivazquez.net> 0.12.2-6
- Fix build on x86_64

* Sat Mar 19 2005 Ignacio Vazquez-Abrams <ivazquez@ivazquez.net> 0.12.2-5
- %%

* Sat Mar 19 2005 Ignacio Vazquez-Abrams <ivazquez@ivazquez.net> 0.12.2-4
- Used %%find_lang
- Tightened up %%files
- Streamlined spec file

* Thu Mar 17 2005 Ignacio Vazquez-Abrams <ivazquez@ivazquez.net> 0.12.2-3
- Broke %%description at 80 columns

* Wed Mar 16 2005 Ignacio Vazquez-Abrams <ivazquez@ivazquez.net> 0.12.2-2
- Removed explicit Requires

* Tue Mar 15 2005 Ignacio Vazquez-Abrams <ivazquez@ivazquez.net> 0.12.2-1
- Bump release to 1
- Cleaned up the -docs and -speex patches

* Fri Jan 21 2005 Ignacio Vazquez-Abrams <ivazquez@ivazquez.net> 0:0.12.2-0.iva.1
- Fixed a silly spec error

* Fri Jan 21 2005 Ignacio Vazquez-Abrams <ivazquez@ivazquez.net> 0:0.12.2-0.iva.0
- Initial RPM release.
