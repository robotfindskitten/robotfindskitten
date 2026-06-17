# SPDX-PackageName: robotfindskitten
# SPDX-PackageDownloadLocation: https://codeberg.org/robotfindskitten/robotfindskitten
# SPDX-FileCopyrightText: © 1997 Leonard Richardson and contributors
# SPDX-License-Identifier: GPL-2.0-or-later

dnl Process this file with autoconf to produce a configure script.
AC_INIT([robotfindskitten],[3.0000000.726],[ryan@finnie.org],[robotfindskitten])
AM_INIT_AUTOMAKE([gnu])
dnl Make builds less verbose.  Shuts off Makefile portability checks.
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_CONFIG_MACRO_DIRS([m4])
AC_CONFIG_HEADERS(config.h)
AC_LANG([C])

dnl Checks for programs.
AC_PROG_CC
LT_INIT
AC_PROG_INSTALL

dnl Checks for libraries.
AC_CHECK_LIB(ncurses, initscr, ,curses)

dnl Checks for header files.
AC_CHECK_HEADERS(signal.h)
AC_CHECK_HEADERS(ncurses.h,,
	[AC_CHECK_HEADERS(curses.h,,
		AC_MSG_ERROR(Could not find ncurses headers!))])

AC_CONFIG_FILES([Makefile src/Makefile doc/Makefile nki/Makefile share/Makefile])
AC_OUTPUT
