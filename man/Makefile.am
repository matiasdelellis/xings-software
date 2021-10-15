EXTRA_DIST =						\
	xings-software.sgml				\
	xings-packagekit-service.sgml			\
	xings-install-local-package.sgml		\
	xings-install-package-name.sgml			\
	xings-software-history.sgml			\
	xings-packagekit-preferences.sgml			\
	xings-software-update.sgml

if HAVE_DOCBOOK2MAN
man_MANS =						\
	xings-software.1				\
	xings-packagekit-service.1			\
	xings-install-local-package.1			\
	xings-install-package-name.1			\
	xings-software-history.1			\
	xings-packagekit-preferences.1			\
	xings-software-update.1
endif

if HAVE_DOCBOOK2MAN
xings-software.1: xings-software.sgml
	$(AM_V_GEN) docbook2man $? > /dev/null
xings-install-local-package.1: xings-install-local-package.sgml
	$(AM_V_GEN) docbook2man $? > /dev/null
xings-packagekit-preferences.1: xings-packagekit-preferences.sgml
	$(AM_V_GEN) docbook2man $? > /dev/null
xings-install-package-name.1: xings-install-package-name.sgml
	$(AM_V_GEN) docbook2man $? > /dev/null
xings-packagekit-service.1: xings-packagekit-service.sgml
	$(AM_V_GEN) docbook2man $? > /dev/null
xings-software-update.1: xings-software-update.sgml
	$(AM_V_GEN) docbook2man $? > /dev/null
xings-software-history.1: xings-software-history.sgml
	$(AM_V_GEN) docbook2man $? > /dev/null
endif

clean-local :
	rm -f *~
	rm -f *.1
	rm -f manpage.*

