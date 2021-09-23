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
	-DG_LOG_DOMAIN=\"XingsPackageKit\"

bin_PROGRAMS = xings-updates-service

xings_updates_service_SOURCES =				\
	gpk-updates-manager.c				\
	gpk-updates-manager.h				\
	gpk-updates-notification.c			\
	gpk-updates-notification.h			\
	gpk-updates-applet.c				\
	gpk-updates-applet.h				\
	gpk-updates-service.c

xings_updates_service_LDADD =				\
	$(top_builddir)/src/common/libgpk-common.a	\
	$(PACKAGEKIT_LIBS)				\
	$(STATUSNOTIFIER_LIBS)				\
	$(GLIB_LIBS)					\
	$(GIO_LIBS)					\
	$(DBUS_LIBS)					\
	$(NOTIFY_LIBS)					\
	$(X11_LIBS)

xings_updates_services_CFLAGS = 			\
	$(STATUSNOTIFIER_CFLAGS)			\
	$(WARN_CFLAGS)
