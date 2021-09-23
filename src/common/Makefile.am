AM_CPPFLAGS =						\
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

noinst_LIBRARIES = libgpk-common.a

libgpk_common_a_SOURCES =				\
	gpk-debug.c					\
	gpk-debug.h					\
	gpk-enum.c					\
	gpk-enum.h					\
	gpk-x11.c					\
	gpk-x11.h					\
	gpk-marshal.c					\
	gpk-marshal.h					\
	gpk-session.c					\
	gpk-session.h					\
	gpk-desktop.c					\
	gpk-desktop.h					\
	gpk-dialog.c					\
	gpk-dialog.h					\
	gpk-vendor.c					\
	gpk-vendor.h					\
	gpk-language.c					\
	gpk-language.h					\
	gpk-modal-dialog.c				\
	gpk-modal-dialog.h				\
	gpk-helper-run.c				\
	gpk-helper-run.h				\
	gpk-helper-chooser.c				\
	gpk-helper-chooser.h				\
	gpk-gnome.c					\
	gpk-gnome.h					\
	gpk-common.c					\
	gpk-common.h					\
	gpk-task.c					\
	gpk-task.h					\
	gpk-error.c					\
	gpk-error.h

if WITH_SYSTEMD
libgpk_common_a_SOURCES +=				\
	systemd-proxy.c					\
	systemd-proxy.h
endif

libgpk_common_a_LIBS =					\
	$(GLIB_LIBS)					\
	$(GIO_LIBS)					\
	$(DBUS_LIBS)					\
	$(GTK_LIBS)					\
	$(NOTIFY_LIBS)					\
	$(SYSTEMD_LIBS)					\
	$(PACKAGEKIT_LIBS)				\
	$(X11_LIBS)

libgpk_common_a_CFLAGS =					\
	$(WARN_CFLAGS)

BUILT_SOURCES = 					\
	gpk-marshal.c					\
	gpk-marshal.h

gpk-marshal.c: gpk-marshal.list
	echo "#include \"gpk-marshal.h\"" > $@ && \
	glib-genmarshal $< --prefix=gpk_marshal --body >> $@

gpk-marshal.h: gpk-marshal.list
	glib-genmarshal $< --prefix=gpk_marshal --header > $@

EXTRA_DIST =						\
	gpk-marshal.list

clean-local:
	rm -f *~
	rm -f gpk-marshal.c gpk-marshal.h
