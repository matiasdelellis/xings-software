themedir = $(pkgdatadir)/icons/hicolor
size = 22x22
context = status

iconsdir = $(themedir)/$(size)/$(context)

icons_DATA = 				\
	xings-collection-installed.png	\
	xings-collection-available.png	\
	xings-package-installed.png	\
	xings-package-available.png	\
	xings-package-add.png		\
	xings-package-cleanup.png	\
	xings-package-blocked.png	\
	xings-package-delete.png	\
	xings-package-download.png	\
	xings-package-downloaded.png	\
	xings-package-search.png	\
	xings-package-update.png	\
	xings-package-info.png		\
	xings-package-sources.png	\
	xings-update-bugfix.png		\
	xings-update-enhancement.png	\
	xings-update-high.png		\
	xings-update-low.png		\
	xings-update-normal.png		\
	xings-update-security.png	\
	xings-refresh-cache.png		\
	xings-package-setup.png		\
	xings-package-wait.png

install-data-hook:
	mkdir -p $(DESTDIR)$(DATADIR)/icons/hicolor/$(size)/apps;
	ln -fs $(iconsdir)/xings-package-sources.svg $(DESTDIR)$(DATADIR)/icons/hicolor/$(size)/apps/xings-software-preferences.svg;
	ln -fs $(iconsdir)/xings-package-info.svg $(DESTDIR)$(DATADIR)/icons/hicolor/$(size)/apps/xings-software-history.svg

EXTRA_DIST =				\
	$(icons_DATA)


