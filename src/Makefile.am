## We require new-style dependency handling.
AUTOMAKE_OPTIONS = 1.7

AM_CPPFLAGS =						\
	$(GLIB_CFLAGS)					\
	$(GIO_CFLAGS)					\
	$(GTK_CFLAGS)					\
	$(DBUS_CFLAGS)					\
	$(NOTIFY_CFLAGS)				\
	$(PACKAGEKIT_CFLAGS)				\
	$(GUDEV_CFLAGS)					\
	$(SYSTEMD_CFLAGS)				\
	$(GNOME_MENUS_CFLAGS)				\
	-DI_KNOW_THE_PACKAGEKIT_GLIB2_API_IS_SUBJECT_TO_CHANGE	\
	-DG_UDEV_API_IS_SUBJECT_TO_CHANGE		\
	$(CANBERRA_CFLAGS)				\
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
	xings-packages					\
	xings-preferences				\
	xings-install-local-package			\
	xings-install-package-name			\
	xings-package-updates				\
	xings-package-history				\
	xings-packagekit-service

noinst_LIBRARIES = libgpkshared.a
libgpkshared_a_SOURCES =				\
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
	gpk-scheduler.c					\
	gpk-scheduler.h					\
	gpk-task.c					\
	gpk-task.h					\
	gpk-error.c					\
	gpk-error.h

if WITH_SYSTEMD
libgpkshared_a_SOURCES +=				\
	systemd-proxy.c					\
	systemd-proxy.h
endif

shared_LIBS =						\
	$(GLIB_LIBS)					\
	$(GIO_LIBS)					\
	$(DBUS_LIBS)					\
	$(GTK_LIBS)					\
	$(GUDEV_LIBS)					\
	$(NOTIFY_LIBS)					\
	$(SYSTEMD_LIBS)					\
	$(PACKAGEKIT_LIBS)				\
	$(GNOME_MENUS_LIBS)				\
	$(CANBERRA_LIBS)				\
	$(X11_LIBS)

xings_install_local_package_SOURCES =			\
	gpk-install-local-file.c			\
	$(NULL)

xings_install_local_package_LDADD =			\
	libgpkshared.a					\
	$(shared_LIBS)					\
	$(NULL)

xings_install_package_name_SOURCES =			\
	gpk-install-package-name.c			\
	$(NULL)

xings_install_package_name_LDADD =			\
	libgpkshared.a					\
	$(shared_LIBS)					\
	$(NULL)

xings_packagekit_service_SOURCES =			\
	gpk-dbus-service.c				\
	gpk-dbus.c					\
	gpk-dbus.h					\
	gpk-dbus-task.c					\
	gpk-dbus-task.h					\
	$(NULL)

xings_packagekit_service_LDADD =			\
	libgpkshared.a					\
	$(shared_LIBS)					\
	$(NULL)

xings_packages_SOURCES =				\
	gpk-application.c

xings_packages_LDADD =					\
	libgpkshared.a					\
	$(shared_LIBS)

xings_packages_CFLAGS =					\
	$(WARN_CFLAGS)

xings_preferences_SOURCES =				\
	gpk-debug.h					\
	gpk-debug.c					\
	gpk-enum.c					\
	gpk-enum.h					\
	gpk-common.c					\
	gpk-common.h					\
	gpk-error.c					\
	gpk-error.h					\
	gpk-prefs.c

xings_preferences_LDADD =				\
	libgpkshared.a					\
	$(shared_LIBS)					\
	-lm

xings_package_updates_SOURCES =				\
	gpk-update-viewer.c				\
	gpk-cell-renderer-size.c			\
	gpk-cell-renderer-size.h			\
	gpk-cell-renderer-info.c			\
	gpk-cell-renderer-info.h			\
	gpk-cell-renderer-restart.c			\
	gpk-cell-renderer-restart.h

xings_package_updates_LDADD =				\
	libgpkshared.a					\
	$(shared_LIBS)

gpk_update_viewer_CFLAGS =				\
	$(WARN_CFLAGS)

xings_package_history_SOURCES =				\
	gpk-log.c

xings_package_history_LDADD =				\
	libgpkshared.a					\
	$(shared_LIBS)

gpk_log_CFLAGS =					\
	$(WARN_CFLAGS)

BUILT_SOURCES = 					\
	gpk-marshal.c					\
	gpk-marshal.h					\
	org.freedesktop.PackageKit.h			\
	$(NULL)

gpk-marshal.c: gpk-marshal.list
	echo "#include \"gpk-marshal.h\"" > $@ && \
	glib-genmarshal $< --prefix=gpk_marshal --body >> $@

gpk-marshal.h: gpk-marshal.list
	glib-genmarshal $< --prefix=gpk_marshal --header > $@

org.freedesktop.PackageKit.h: org.freedesktop.PackageKit.xml
	$(LIBTOOL) --mode=execute dbus-binding-tool	\
		--prefix=gpk_dbus			\
		--mode=glib-server			\
		--output=org.freedesktop.PackageKit.h	\
		$(srcdir)/org.freedesktop.PackageKit.xml

if EGG_BUILD_TESTS

check_PROGRAMS =					\
	xings-self-test

noinst_PROGRAMS =					\
	xings-self-test

xings_self_test_SOURCES =				\
	gpk-self-test.c					\
	gpk-debug.c					\
	gpk-debug.h					\
	gpk-enum.c					\
	gpk-enum.h					\
	gpk-common.c					\
	gpk-common.h					\
	gpk-error.c					\
	gpk-error.h					\
	gpk-language.c					\
	gpk-language.h					\
	gpk-modal-dialog.c				\
	gpk-modal-dialog.h				\
	gpk-dbus.c					\
	gpk-dbus.h					\
	gpk-dbus-task.c					\
	gpk-dbus-task.h					\
	gpk-gnome.c					\
	gpk-gnome.h					\
	gpk-x11.c					\
	gpk-x11.h					\
	gpk-vendor.c					\
	gpk-vendor.h					\
	gpk-desktop.c					\
	gpk-desktop.h					\
	gpk-task.c					\
	gpk-task.h					\
	gpk-helper-run.c				\
	gpk-helper-run.h				\
	gpk-helper-chooser.c				\
	gpk-helper-chooser.h				\
	gpk-marshal.c					\
	gpk-marshal.h					\
	gpk-dialog.c					\
	gpk-dialog.h

xings_self_test_LDADD =					\
	$(shared_LIBS)					\
	$(SELFTEST_LIBS)

xings_self_test_CFLAGS = $(AM_CFLAGS)

TESTS = xings-self-test
endif

EXTRA_DIST =						\
	gpk-marshal.list				\
	org.freedesktop.PackageKit.xml			\
	$(NULL)

clean-local:
	rm -f *~
	rm -f gpk-marshal.c gpk-marshal.h
	rm -f *.out
	rm -f *.gcda
	rm -f *.gcno
	rm -f gcov.txt
	rm -f gprof.txt

CLEANFILES = *~ $(BUILT_SOURCES)

MAINTAINERCLEANFILES =					\
	*~			      			\
	Makefile.in

-include $(top_srcdir)/git.mk
