NULL =

SUBDIRS = appdata categories icons

@INTLTOOL_DESKTOP_RULE@
desktopdir = $(datadir)/applications
desktop_in_files =					\
	xings-install-local-package.desktop.in		\
	xings-packagekit-service.desktop.in		\
	xings-software.desktop.in			\
	xings-software-history.desktop.in		\
	xings-software-preferences.desktop.in		\
	xings-software-update.desktop.in		\
	$(NULL)

desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

@INTLTOOL_DESKTOP_RULE@
autostartdir = $(sysconfdir)/xdg/autostart
autostart_in_files =					\
	xings-software-service.desktop.in

autostart_DATA = $(autostart_in_files:.desktop.in=.desktop)

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
service_in_files = org.xings.PackageKit.service.in
service_DATA = $(service_in_files:.service.in=.service)

$(service_DATA): $(service_in_files) Makefile
	@sed -e "s|\@servicedir\@|$(bindir)|" $< > $@

@GSETTINGS_RULES@
gsettings_SCHEMAS = org.xings.software.gschema.xml

EXTRA_DIST =						\
	$(gsettings_SCHEMAS)				\
	$(autostart_in_files)				\
	$(desktop_in_files)				\
	$(pkgdata_DATA)					\
	$(service_in_files)				\
	$(NULL)

DISTCLEANFILES =					\
	xings-install-local-package.desktop		\
	xings-packagekit-service.desktop		\
	xings-software.desktop				\
	xings-software-history.desktop			\
	xings-software-preferences.desktop		\
	xings-software-update.desktop			\
	xings-software-service.desktop			\
	org.xings.PackageKit.service		\
	$(NULL)

clean-local :
	rm -f *~

