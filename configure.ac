dnl  Copyright 2005 Red Hat, Inc.
dnl
dnl  Permission to use, copy, modify, distribute, and sell this software and its
dnl  documentation for any purpose is hereby granted without fee, provided that
dnl  the above copyright notice appear in all copies and that both that
dnl  copyright notice and this permission notice appear in supporting
dnl  documentation, and that the name of Red Hat not be used in
dnl  advertising or publicity pertaining to distribution of the software without
dnl  specific, written prior permission.  Red Hat makes no
dnl  representations about the suitability of this software for any purpose.  It
dnl  is provided "as is" without express or implied warranty.
dnl
dnl  RED HAT DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
dnl  INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
dnl  EVENT SHALL RED HAT BE LIABLE FOR ANY SPECIAL, INDIRECT OR
dnl  CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
dnl  DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
dnl  TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
dnl  PERFORMANCE OF THIS SOFTWARE.
dnl
dnl Process this file with autoconf to create configure.

# Initialize Autoconf
AC_PREREQ([2.60])
AC_INIT([xmcalc], [1.1.3],
        [https://gitlab.freedesktop.org/xorg/app/xmcalc/-/issues], [xmcalc])
AC_CONFIG_SRCDIR([Makefile.am])
AC_CONFIG_HEADERS([config.h])

# Initialize Automake
AM_INIT_AUTOMAKE([foreign dist-xz])

# Compiler checks
AC_PROG_CC
AC_PROG_SED

# Checks for library functions
AC_CHECK_FUNCS([strlcpy])

# Checks for pkg-config packages - Motif/Xm dependency
# Try 'xm' first, then 'motif', then fallback to AC_CHECK_LIB
PKG_CHECK_MODULES(XMCALC, [xm xt x11 xproto >= 7.0.22],
  [have_xm_pkgconfig=yes],
  [PKG_CHECK_MODULES(XMCALC, [motif xt x11 xproto >= 7.0.22],
     [have_xm_pkgconfig=yes],
     [have_xm_pkgconfig=no])])

if test "x$have_xm_pkgconfig" = "xno"; then
  AC_CHECK_LIB([Xm], [XmCreateForm],
    [XMCALC_LIBS="-lXm -lXt -lX11"
     XMCALC_CFLAGS=""],
    [AC_MSG_ERROR([No Motif/Xm library found])])
  AC_CHECK_HEADERS([Xm/Xm.h],,
    [AC_MSG_ERROR([No Xm/Xm.h header found])])
fi

AC_SUBST(XMCALC_CFLAGS)
AC_SUBST(XMCALC_LIBS)

# App-defaults directory
PKG_CHECK_MODULES([APPDEFS], [xt],,
  [APPDEFS_CFLAGS=""
   APPDEFS_LIBS="-lXt"])

xt_appdefaultdir=`$PKG_CONFIG --variable=appdefaultdir xt 2>/dev/null`
test -z "$xt_appdefaultdir" && xt_appdefaultdir="${datadir}/X11/app-defaults"
AC_ARG_WITH(appdefaultdir,
	AS_HELP_STRING([--with-appdefaultdir=<pathname>],
	  [specify directory for app-defaults files (default is autodetected)]),
	[appdefaultdir="$withval"], [appdefaultdir="${xt_appdefaultdir}"])
AC_SUBST(appdefaultdir)

# Man page substitutions
APP_MAN_SUFFIX=1
APP_MAN_DIR='$(datadir)/man/man1'
FILE_MAN_SUFFIX=5
FILE_MAN_DIR='$(datadir)/man/man5'
MISC_MAN_SUFFIX=7
MISC_MAN_DIR='$(datadir)/man/man7'
AC_SUBST([APP_MAN_SUFFIX])
AC_SUBST([APP_MAN_DIR])
AC_SUBST([FILE_MAN_SUFFIX])
AC_SUBST([FILE_MAN_DIR])
AC_SUBST([MISC_MAN_SUFFIX])
AC_SUBST([MISC_MAN_DIR])

MAN_SUBSTS="-e 's|__appmansuffix__|${APP_MAN_SUFFIX}|g' -e 's|__filemansuffix__|${FILE_MAN_SUFFIX}|g' -e 's|__libmansuffix__|3|g' -e 's|__miscmansuffix__|${MISC_MAN_SUFFIX}|g' -e 's|__drivermansuffix__|4|g' -e 's|__adminmansuffix__|8|g' -e 's|__xorgversion__|xmcalc 1.1.3|g' -e 's|__apploaddir__|${appdefaultdir}|g'"
AC_SUBST([MAN_SUBSTS])

AC_CONFIG_FILES([
	Makefile
	man/Makefile])
AC_OUTPUT