EXTRA_DIST =						\
	xings-packages.sgml				\
	xings-packagekit-service.sgml			\
	xings-install-local-package.sgml		\
	xings-install-package-name.sgml			\
	xings-package-history.sgml			\
	xings-preferences.sgml				\
	xings-package-updates.sgml

if HAVE_DOCBOOK2MAN
man_MANS =						\
	xings-packages.1				\
	xings-packagekit-service.1			\
	xings-install-local-package.1			\
	xings-install-package-name.1			\
	xings-package-history.1				\
	xings-preferences.1				\
	xings-package-updates.1
endif

if HAVE_DOCBOOK2MAN
xings-packages.1: xings-packages.sgml
	docbook2man $? > /dev/null
xings-install-local-package.1: xings-install-local-package.sgml
	docbook2man $? > /dev/null
xings-preferences.1: xings-preferences.sgml
	docbook2man $? > /dev/null
xings-install-package-name.1: xings-install-package-name.sgml
	docbook2man $? > /dev/null
xings-packagekit-service.1: xings-packagekit-service.sgml
	docbook2man $? > /dev/null
xings-package-updates.1: xings-package-updates.sgml
	docbook2man $? > /dev/null
xings-package-history.1: xings-package-history.sgml
	docbook2man $? > /dev/null
endif

clean-local :
	rm -f *~
	rm -f *.1
	rm -f manpage.*

