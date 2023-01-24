categorydir = $(pkgdatadir)/categories
category_in_files =							\
	accesories.directory.in						\
	development.directory.in					\
	education.directory.in						\
	games.directory.in						\
	graphics.directory.in						\
	multimedia.directory.in						\
	network.directory.in						\
	office.directory.in						\
	science.directory.in						\
	settings.directory.in

category_DATA = $(category_in_files:.directory.in=.directory)
@INTLTOOL_DIRECTORY_RULE@

EXTRA_DIST =								\
	$(category_in_files)

DISTCLEANFILES =							\
	$(category_DATA)
