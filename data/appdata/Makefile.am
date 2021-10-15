@INTLTOOL_XML_RULE@
appdatadir = $(datadir)/metainfo
appdata_DATA = $(appdata_in_files:.xml.in=.xml)
appdata_in_files =					\
	xings-packages.appdata.xml.in		\
	xings-package-updates.appdata.xml.in
EXTRA_DIST = $(appdata_in_files)
CLEANFILES = $(appdata_DATA)

