themedir = $(pkgdatadir)/icons/hicolor
size = 16x16
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
	xings-setup.png			\
	xings-wait.png

install-data-hook:
	mkdir -p $(DESTDIR)$(DATADIR)/icons/hicolor/$(size)/apps;
	ln -fs $(iconsdir)/xings-package-sources.png $(DESTDIR)$(DATADIR)/icons/hicolor/$(size)/apps/xings-repo.png;
	ln -fs $(iconsdir)/xings-package-info.png $(DESTDIR)$(DATADIR)/icons/hicolor/$(size)/apps/xings-software-history.png;
	ln -fs $(iconsdir)/xings-update-high.png $(DESTDIR)$(DATADIR)/icons/hicolor/$(size)/apps/xings-packagekit-preferences.png;

EXTRA_DIST =				\
	$(icons_DATA)


