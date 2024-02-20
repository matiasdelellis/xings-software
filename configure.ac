AC_PREREQ(2.65)

AC_INIT([xings-software],[0.3.94])
AC_CONFIG_SRCDIR(src)
AM_INIT_AUTOMAKE([1.9 no-dist-gzip dist-xz tar-ustar])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

# enable nice build output on automake1.11
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

AC_PROG_CC
AC_PROG_INSTALL
LT_INIT
AM_PROG_CC_C_O
IT_PROG_INTLTOOL([0.35.0])

GNOME_COMPILE_WARNINGS

# set up gtk-doc
GTK_DOC_CHECK(1.9)

AC_PATH_PROG(GLIB_GENMARSHAL, glib-genmarshal)

WARN_CFLAGS_EXTRA="
	-Waggregate-return
	-Warray-bounds
	-Wcast-align
	-Wclobbered
	-Wdeclaration-after-statement
	-Wempty-body
	-Wextra
	-Wformat=2
	-Wformat-nonliteral
	-Wformat-security
	-Wformat-signedness
	-Wignored-qualifiers
	-Wimplicit-function-declaration
	-Winit-self
	-Winline
	-Wmissing-declarations
	-Wmissing-format-attribute
	-Wmissing-include-dirs
	-Wmissing-noreturn
	-Wmissing-parameter-type
	-Wmissing-prototypes
	-Wnested-externs
	-Wno-discarded-qualifiers
	-Wno-missing-field-initializers
	-Wno-strict-aliasing
	-Wno-suggest-attribute=format
	-Wno-unused-parameter
	-Wold-style-definition
	-Woverride-init
	-Wpacked
	-Wpointer-arith
	-Wredundant-decls
	-Wreturn-type
	-Wshadow
	-Wsign-compare
	-Wstrict-aliasing
	-Wstrict-prototypes
	-Wswitch-default
	-Wtype-limits
	-Wundef
	-Wuninitialized
	-Wunused-but-set-variable
	-Wwrite-strings
	-Wunreachable-code"
AX_APPEND_COMPILE_FLAGS([$WARN_CFLAGS_EXTRA], [WARN_CFLAGS])

dnl ---------------------------------------------------------------------------
dnl - xsltproc
dnl ---------------------------------------------------------------------------
AC_PATH_PROG(XSLTPROC, xsltproc, no)
if test "$XSLTPROC" = "no" ; then
	AC_MSG_ERROR([xsltproc has not been found])
fi

dnl ---------------------------------------------------------------------------
dnl - gettext stuff
dnl ---------------------------------------------------------------------------
GETTEXT_PACKAGE=AC_PACKAGE_NAME
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE(GETTEXT_PACKAGE, "AC_PACKAGE_NAME", [foo])

AM_GLIB_GNU_GETTEXT
GLIB_GSETTINGS

dnl ---------------------------------------------------------------------------
dnl - Check library dependencies
dnl ---------------------------------------------------------------------------
PKG_CHECK_MODULES(PACKAGEKIT, packagekit-glib2 >= 0.9.1)
PKG_CHECK_MODULES(APPSTREAM, appstream >= 0.12.5)
PKG_CHECK_MODULES(GLIB, \
 glib-2.0 >= 2.32.0
 gobject-2.0
 gio-2.0 >= 2.25.9
 gio-unix-2.0)
PKG_CHECK_MODULES(DBUS, \
 dbus-glib-1 >= 0.73 \
 dbus-1 >= 1.1.2 \
 gthread-2.0)
PKG_CHECK_MODULES(GTK, \
 gtk+-3.0 >= 3.15.2 gdk-3.0 fontconfig)
PKG_CHECK_MODULES(X11, x11)
PKG_CHECK_MODULES(FLATPAK, flatpak)

dnl **** Check for libnotify ****
PKG_CHECK_MODULES(NOTIFY, libnotify >= 0.7.0)

dnl ---------------------------------------------------------------------------
dnl systemd integration
dnl ---------------------------------------------------------------------------
AC_ARG_ENABLE([systemd],
              AS_HELP_STRING([--enable-systemd], [Use systemd]),
              [with_systemd=$enableval],
              [with_systemd=auto])

PKG_CHECK_MODULES(SYSTEMD,
                  [libsystemd polkit-gobject-1],
                  [have_systemd=yes], [have_systemd=no])

AC_MSG_CHECKING([whether to use systemd])

if test x$with_systemd = xauto ; then
        if test x$have_systemd = xno ; then
                with_systemd=no
        else
                with_systemd=yes
        fi
fi

AC_MSG_RESULT($with_systemd)

if test x$with_systemd = xyes; then
        if test x$have_systemd = xno; then
                AC_MSG_ERROR([Systemd support explicitly required, but systemd not found])
        fi
        AC_DEFINE(HAVE_SYSTEMD, 1, [Define if systemd is used for session tracking])
fi

AC_SUBST(SYSTEMD_CFLAGS)
AC_SUBST(SYSTEMD_LIBS)

AM_CONDITIONAL(WITH_SYSTEMD, [test "$with_systemd" = "yes"], [Using systemd])


dnl ---------------------------------------------------------------------------
dnl - Application indicator
dnl ---------------------------------------------------------------------------

STATUSNOTIFIER_PKG=statusnotifier
STATUSNOTIFIER_REQUIRED=1.0.0
AC_ARG_ENABLE([statusnotifier],
        AS_HELP_STRING([--enable-statusnotifier[=@<:@no/auto/yes@:>@]],[Build support for status notifier]),
                       [enable_statusnotifier=$enableval],
                       [enable_statusnotifier="auto"])

if test "x$enable_statusnotifier" = "xauto"; then
        PKG_CHECK_EXISTS($STATUSNOTIFIER_PKG >= $STATUSNOTIFIER_REQUIRED,
                         [enable_statusnotifier="yes"],
                         [enable_statusnotifier="no"])
fi

if test "x$enable_statusnotifier" = "xyes"; then
        PKG_CHECK_MODULES(STATUSNOTIFIER,
                          [$STATUSNOTIFIER_PKG >= $STATUSNOTIFIER_REQUIRED],
                          [AC_DEFINE(HAVE_STATUSNOTIFIER, 1, [Have StatusNotifier])])
fi

AM_CONDITIONAL(HAVE_STATUSNOTIFIER, test "x$enable_statusnotifier" = "xyes")
AC_SUBST(STATUSNOTIFIER_CFLAGS)
AC_SUBST(STATUSNOTIFIER_LIBS)

dnl ---------------------------------------------------------------------------
dnl - Enable offline updates
dnl ---------------------------------------------------------------------------
AC_ARG_ENABLE([offline_updates],
              AS_HELP_STRING([--enable-offline-updates], [Prefer Offline Updates]),
              [with_offline_updates=$enableval],
              [with_offline_updates=no])

if test x$with_offline_updates = xyes ; then
	AC_DEFINE(BUILD_OFFLINE_UPDATES, 1, [Define if is targered an environment that can reboot to install updates])
fi

dnl ---------------------------------------------------------------------------
dnl - Enable small form factor code
dnl ---------------------------------------------------------------------------
AC_ARG_ENABLE(small_form_factor, AS_HELP_STRING([--enable-small-form-factor],[enable small form factor code]),
	      enable_small_form_factor=$enableval,enable_small_form_factor=no)
AM_CONDITIONAL(PK_BUILD_SMALL_FORM_FACTOR, test x$enable_small_form_factor = xyes)
if test x$enable_small_form_factor = xyes; then
	AC_DEFINE(PK_BUILD_SMALL_FORM_FACTOR,1,[Build small form factor code])
fi

dnl ---------------------------------------------------------------------------
dnl - Make paths available for source files
dnl ---------------------------------------------------------------------------
AC_SUBST(SYSCONFDIR, $sysconfdir)
AC_SUBST(DATADIR, $datadir)
AC_SUBST(BINDIR, $bindir)
AC_SUBST(SBINDIR, $sbindir)

dnl ---------------------------------------------------------------------------
dnl - Is docbook2man available?
dnl ---------------------------------------------------------------------------
AC_PATH_PROG(DOCBOOK2MAN, docbook2man, no)
if test "$DOCBOOK2MAN" = "no" ; then
	AC_MSG_WARN([docbook2man not found, will not be able to build man documentation])
fi
AM_CONDITIONAL(HAVE_DOCBOOK2MAN, [test "$DOCBOOK2MAN" != "no"])

dnl ---------------------------------------------------------------------------
dnl - Build self tests
dnl ---------------------------------------------------------------------------
AC_ARG_ENABLE(tests, AS_HELP_STRING([--enable-tests],[enable unit test code]),
	      enable_tests=$enableval,enable_tests=no)
AM_CONDITIONAL(EGG_BUILD_TESTS, test x$enable_tests = xyes)
if test x$enable_tests = xyes; then
	AC_DEFINE(EGG_BUILD_TESTS,1,[Build test code])
fi

dnl ---------------------------------------------------------------------------
dnl - Makefiles, etc.
dnl ---------------------------------------------------------------------------
AC_CONFIG_FILES([
Makefile
src/Makefile
src/common/Makefile
src/helpers/Makefile
src/packagekit-service/Makefile
src/preferences/Makefile
src/software/Makefile
src/software-service/Makefile
src/software-update/Makefile
src/third-parties/Makefile
man/Makefile
po/Makefile.in
data/Makefile
data/appdata/Makefile
data/categories/Makefile
docs/Makefile
docs/dbus/Makefile
data/icons/Makefile
data/icons/16x16/Makefile
data/icons/22x22/Makefile
data/icons/24x24/Makefile
data/icons/48x48/Makefile
data/icons/scalable/Makefile
data/icons/16x16/categories/Makefile
data/icons/16x16/status/Makefile
data/icons/22x22/categories/Makefile
data/icons/22x22/status/Makefile
data/icons/24x24/categories/Makefile
data/icons/24x24/status/Makefile
data/icons/48x48/categories/Makefile
data/icons/48x48/status/Makefile
data/icons/scalable/categories/Makefile
data/icons/scalable/status/Makefile
])
AC_OUTPUT

dnl ==========================================================================
echo "
                    xings-software v$VERSION
                  =============================

        prefix:                    ${prefix}
        compiler:                  ${CC}
        cflags:                    ${CFLAGS}
        cppflags:                  ${CPPFLAGS}

        Prefer Offline Updates:    ${with_offline_updates}

        Systemd Integrations:      ${have_systemd}

        Status Notifier:           ${enable_statusnotifier}
"
