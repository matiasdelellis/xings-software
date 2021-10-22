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

bin_PROGRAMS = xings-software-service

xings_software_service_SOURCES =				\
	gpk-updates-checker.c				\
	gpk-updates-checker.h				\
	gpk-updates-download.c				\
	gpk-updates-download.h				\
	gpk-updates-manager.c				\
	gpk-updates-manager.h				\
	gpk-updates-notification.c			\
	gpk-updates-notification.h			\
	gpk-updates-refresh.c				\
	gpk-updates-refresh.h				\
	gpk-updates-shared.c				\
	gpk-updates-shared.h				\
	gpk-updates-service.c

xings_software_service_LDADD =				\
	$(top_builddir)/src/common/libgpk-common.a	\
	$(PACKAGEKIT_LIBS)				\
	$(STATUSNOTIFIER_LIBS)				\
	$(GLIB_LIBS)					\
	$(GIO_LIBS)					\
	$(DBUS_LIBS)					\
	$(NOTIFY_LIBS)					\
	$(SYSTEMD_LIBS)					\
	$(X11_LIBS)

xings_software_services_CFLAGS = 			\
	$(STATUSNOTIFIER_CFLAGS)			\
	$(WARN_CFLAGS)
