NULL =

SUBDIRS = appdata icons

@INTLTOOL_DESKTOP_RULE@
desktopdir = $(datadir)/applications
desktop_in_files =					\
	xings-install-local-package.desktop.in		\
	xings-packagekit-service.desktop.in		\
	xings-preferences.desktop.in			\
	xings-packages.desktop.in			\
	xings-package-updates.desktop.in		\
	xings-package-history.desktop.in		\
	$(NULL)

desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

@INTLTOOL_DESKTOP_RULE@

pkgdata_DATA =						\
	gpk-eula.ui					\
	gpk-application.ui				\
	gpk-update-viewer.ui				\
	gpk-prefs.ui					\
	gpk-signature.ui				\
	gpk-error.ui					\
	gpk-log.ui					\
	gpk-client.ui					\
	$(NULL)

servicedir = $(datadir)/dbus-1/services
service_in_files = org.freedesktop.PackageKit.service.in
service_DATA = $(service_in_files:.service.in=.service)

$(service_DATA): $(service_in_files) Makefile
	@sed -e "s|\@servicedir\@|$(bindir)|" $< > $@

@GSETTINGS_RULES@
gsettings_SCHEMAS = org.xings.packagekit.gschema.xml

EXTRA_DIST =						\
	$(gsettings_SCHEMAS)				\
	$(autostart_in_files)				\
	$(desktop_in_files)				\
	$(pkgdata_DATA)					\
	$(service_in_files)				\
	$(NULL)

clean-local :
	rm -f *~

DISTCLEANFILES =					\
	xings-preferences.desktop			\
	xings-package-updates.desktop			\
	xings-install-local-package.desktop		\
	xings-packagekit-service.desktop		\
	xings-package-history.desktop			\
	xings-packages.desktop				\
	org.freedesktop.PackageKit.service		\
	$(NULL)
