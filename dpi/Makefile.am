AM_CPPFLAGS = \
	-I$(top_srcdir)

downloadsdir = $(libdir)/dillo/dpi/downloads
ftpdir = $(libdir)/dillo/dpi/ftp
hellodir = $(libdir)/dillo/dpi/hello
datauridir = $(libdir)/dillo/dpi/datauri
downloads_PROGRAMS = downloads.dpi
ftp_PROGRAMS = ftp.filter.dpi
hello_PROGRAMS = hello.filter.dpi
datauri_PROGRAMS = datauri.filter.dpi

downloads_dpi_LDADD = @LIBFLTK_LIBS@ \
	$(top_builddir)/dpip/libDpip.a \
	$(top_builddir)/dlib/libDlib.a
ftp_filter_dpi_LDADD = \
	$(top_builddir)/dpip/libDpip.a \
	$(top_builddir)/dlib/libDlib.a
hello_filter_dpi_LDADD = \
	$(top_builddir)/dpip/libDpip.a \
	$(top_builddir)/dlib/libDlib.a
datauri_filter_dpi_LDADD = \
	$(top_builddir)/dpip/libDpip.a \
	$(top_builddir)/dlib/libDlib.a

downloads_dpi_CXXFLAGS = @LIBFLTK_CXXFLAGS@

downloads_dpi_SOURCES = downloads.cc dpiutil.c dpiutil.h
ftp_filter_dpi_SOURCES = ftp.c dpiutil.c dpiutil.h
hello_filter_dpi_SOURCES = hello.c dpiutil.c dpiutil.h
datauri_filter_dpi_SOURCES = datauri.c dpiutil.c dpiutil.h

