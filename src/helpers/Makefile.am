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

bin_PROGRAMS =						\
	xings-install-local-package			\
	xings-install-package-name

xings_install_local_package_SOURCES =			\
	gpk-install-local-file.c			\
	$(NULL)

xings_install_local_package_LDADD =			\
	$(top_builddir)/src/common/libgpk-common.a	\
	$(GLIB_LIBS)					\
	$(GIO_LIBS)					\
	$(DBUS_LIBS)					\
	$(GTK_LIBS)					\
	$(NOTIFY_LIBS)					\
	$(SYSTEMD_LIBS)					\
	$(PACKAGEKIT_LIBS)				\
	$(X11_LIBS)					\
	$(NULL)

xings_install_local_package_CFLAGS =			\
	$(WARN_CFLAGS)

xings_install_package_name_SOURCES =			\
	gpk-install-package-name.c			\
	$(NULL)

xings_install_package_name_LDADD =			\
	$(top_builddir)/src/common/libgpk-common.a	\
	$(GLIB_LIBS)					\
	$(GIO_LIBS)					\
	$(DBUS_LIBS)					\
	$(GTK_LIBS)					\
	$(NOTIFY_LIBS)					\
	$(SYSTEMD_LIBS)					\
	$(PACKAGEKIT_LIBS)				\
	$(X11_LIBS)					\
	$(NULL)

xings_install_package_name_CFLAGS =			\
	$(WARN_CFLAGS)
