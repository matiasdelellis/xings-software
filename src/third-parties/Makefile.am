AM_CPPFLAGS =						\
	-I$(top_srcdir)/src				\
	-I$(top_builddir)/src				\
	$(GLIB_CFLAGS)					\
	$(GIO_CFLAGS)					\
	$(GTK_CFLAGS)					\
	$(DBUS_CFLAGS)					\
	$(NOTIFY_CFLAGS)				\
	$(PACKAGEKIT_CFLAGS)				\
	$(FLATPAK_CFLAGS)				\
	$(SYSTEMD_CFLAGS)				\
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
	xings-third-parties

xings_third_parties_SOURCES =			\
	gpk-flatpak-installer.c			\
	gpk-flatpak-installer.h			\
	gpk-flatpak-ref.c			\
	gpk-flatpak-ref.h			\
	gpk-third-parties.c			\
	$(NULL)

xings_third_parties_LDADD =			\
	$(top_builddir)/src/common/libgpk-common.a	\
	$(GLIB_LIBS)					\
	$(GIO_LIBS)					\
	$(DBUS_LIBS)					\
	$(GTK_LIBS)					\
	$(NOTIFY_LIBS)					\
	$(SYSTEMD_LIBS)					\
	$(PACKAGEKIT_LIBS)				\
	$(FLATPAK_LIBS)					\
	$(X11_LIBS)					\
	$(NULL)

xings_third_parties_CFLAGS =				\
	$(WARN_CFLAGS)
