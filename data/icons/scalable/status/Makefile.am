themedir = $(pkgdatadir)/icons/hicolor
size = scalable
context = status

iconsdir = $(themedir)/$(size)/$(context)

icons_DATA = 				\
	xings-collection-installed.svg	\
	xings-collection-available.svg	\
	xings-package-installed.svg	\
	xings-package-available.svg	\
	xings-package-add.svg		\
	xings-package-cleanup.svg	\
	xings-package-blocked.svg	\
	xings-package-delete.svg	\
	xings-package-download.svg	\
	xings-package-downloaded.svg	\
	xings-package-search.svg	\
	xings-package-update.svg	\
	xings-package-info.svg		\
	xings-package-sources.svg	\
	xings-update-bugfix.svg		\
	xings-update-enhancement.svg	\
	xings-update-high.svg		\
	xings-update-low.svg		\
	xings-update-normal.svg		\
	xings-update-security.svg	\
	xings-refresh-cache.svg		\
	xings-package-setup.svg		\
	xings-package-wait.svg

install-data-hook:
	mkdir -p $(DESTDIR)$(DATADIR)/icons/hicolor/$(size)/apps;
	ln -fs $(iconsdir)/xings-package-sources.svg $(DESTDIR)$(DATADIR)/icons/hicolor/$(size)/apps/xings-software-preferences.svg;
	ln -fs $(iconsdir)/xings-package-info.svg $(DESTDIR)$(DATADIR)/icons/hicolor/$(size)/apps/xings-software-history.svg

EXTRA_DIST =				\
	$(icons_DATA)


