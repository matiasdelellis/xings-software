AM_CPPFLAGS =						\
	-I$(top_srcdir)/src				\
	-I$(top_builddir)/src				\
	$(GLIB_CFLAGS)					\
	$(GIO_CFLAGS)					\
	$(GTK_CFLAGS)					\
	$(DBUS_CFLAGS)					\
	$(NOTIFY_CFLAGS)				\
	$(PACKAGEKIT_CFLAGS)				\
	$(SYSTEMD_CFLAGS)				\
	-DI_KNOW_THE_PACKAGEKIT_GLIB2_API_IS_SUBJECT_TO_CHANGE	\
	-DBINDIR=\"$(bindir)\"			 	\
	-DDATADIR=\"$(datadir)\"			\
	-DPREFIX=\""$(prefix)"\" 			\
	-DSYSCONFDIR=\""$(sysconfdir)"\" 		\
	-DLIBDIR=\""$(libdir)"\" 			\
	-DLIBEXECDIR=\""$(libexecdir)"\" 		\
	-DLOCALEDIR=\""$(localedir)"\"			\
	-DVERSION="\"$(VERSION)\"" 			\
	-DPKGDATADIR=\"$(pkgdatadir)\"			\
	-DG_LOG_DOMAIN=\"XingsSoftware\"

bin_PROGRAMS =						\
	xings-packagekit-service

xings_packagekit_service_SOURCES =			\
	gpk-dbus-service.c				\
	gpk-dbus.c					\
	gpk-dbus.h					\
	gpk-dbus-task.c					\
	gpk-dbus-task.h					\
	$(NULL)

xings_packagekit_service_LDADD =			\
	$(top_builddir)/src/common/libgpk-common.a	\
	$(GLIB_LIBS)					\
	$(GIO_LIBS)					\
	$(DBUS_LIBS)					\
	$(GTK_LIBS)					\
	$(NOTIFY_LIBS)					\
	$(SYSTEMD_LIBS)					\
	$(PACKAGEKIT_LIBS)				\
	$(X11_LIBS)

xings_packagekit_service_CFLAGS =			\
	$(WARN_CFLAGS)


BUILT_SOURCES = 					\
	org.freedesktop.PackageKit.h

org.freedesktop.PackageKit.h: org.freedesktop.PackageKit.xml
	$(LIBTOOL) --mode=execute dbus-binding-tool	\
		--prefix=gpk_dbus			\
		--mode=glib-server			\
		--output=org.freedesktop.PackageKit.h	\
		$(srcdir)/org.freedesktop.PackageKit.xml

EXTRA_DIST =						\
	org.freedesktop.PackageKit.xml

CLEANFILES = *~ $(BUILT_SOURCES)

MAINTAINERCLEANFILES =					\
	*~						\
	Makefile.in
