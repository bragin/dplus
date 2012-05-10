AM_CPPFLAGS= \
	-I$(top_srcdir) \
	-DDILLO_SYSCONF='"$(sysconfdir)/"' \
	@LIBJPEG_CPPFLAGS@
AM_CFLAGS = @LIBPNG_CFLAGS@
AM_CXXFLAGS = @LIBPNG_CFLAGS@ @CURL_CFLAGS@ @LIBFLTK_CXXFLAGS@

SUBDIRS = IO

bin_PROGRAMS = dplus

dplus_LDADD = \
	$(top_builddir)/dlib/libDlib.a \
	IO/libDiof.a \
	$(top_builddir)/dw/libDw-widgets.a \
	$(top_builddir)/dw/libDw-fltk.a \
	$(top_builddir)/dw/libDw-core.a \
	$(top_builddir)/widgets/libDP-widgets.a \
	$(top_builddir)/lout/liblout.a \
	@LIBJPEG_LIBS@ @LIBPNG_LIBS@ @LIBFLTK_LIBS@ @LIBZ_LIBS@ \
	@LIBICONV_LIBS@ @LIBPTHREAD_LIBS@ @CURL_LIBS@

dplus_SOURCES = \
	dplus.cc \
	unicows.h \
	unicows.c \
	paths.cc \
	paths.hh \
	import.c \
	import.h \
	ui.cc \
	ui.hh \
	uicmd.cc \
	uicmd.hh \
	bw.h \
	bw.c \
	cookies.c \
	cookies.h \
	auth.c \
	auth.h \
	colors.c \
	colors.h \
	binaryconst.h \
	misc.c \
	misc.h \
	history.h \
	history.c \
	prefs.c \
	prefs.h \
	prefsparser.cc \
	prefsparser.hh \
	prefsui.cc \
	prefsui.hh \
	keys.cc \
	keys.hh \
	msg.h \
	list.h \
	url.c \
	url.h \
	bitvec.c \
	bitvec.h \
	klist.c \
	klist.h \
	chain.c \
	chain.h \
	utf8.cc \
	utf8.hh \
	timeout.cc \
	timeout.hh \
	dialog.cc \
	dialog.hh \
	icon.c \
	icon.h \
	\
	\
	web.cc \
	web.hh \
	nav.c \
	nav.h \
	cache.c \
	cache.h \
	decode.c \
	decode.h \
	dicache.c \
	dicache.h \
	capi.c \
	capi.h \
	css.cc \
	css.hh \
	cssparser.cc \
	cssparser.hh \
	doctree.hh \
	styleengine.cc \
	styleengine.hh \
	plain.cc \
	html.cc \
	html.hh \
	html_common.hh \
	form.cc \
	form.hh \
	table.cc \
	table.hh \
	bms.h \
	bms.c \
	bookmark.cc \
	bookmark.hh \
	dns.c \
	dns.h \
	dns_alt.c \
	gif.c \
	dgif.h \
	jpeg.c \
	djpeg.h \
	png.c \
	dpng.h \
	imgbuf.cc \
	imgbuf.hh \
	image.cc \
	image.hh \
	menu.hh \
	menu.cc \
	file.c \
	file.h \
	pixmaps.h \
	findbar.cc \
	findbar.hh \
	download.cc \
	download.hh \
	print.cc \
	print.hh \
	xembed.cc \
	xembed.hh

if HAVE_WIN32RES
dplus_LDADD += dplus.res
endif

if HAVE_WIN32RES
dplus.res: dplus.rc dplus.ico
	$(RC) -O coff -o $@ $<
endif

EXTRA_DIST = chg srch
