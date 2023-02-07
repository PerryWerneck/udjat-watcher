
dnl Process this file with autoconf to produce a configure script.

dnl The minimum version of autoconf required.
AC_PREREQ(2.59)

dnl Initialise automake with the package name, version and
dnl bug-reporting address.
AC_INIT([netwatcher], [1.1], [perry.werneck@gmail.com])

dnl Place auxilliary scripts here.
AC_CONFIG_AUX_DIR([scripts])

dnl Compute the canonical host-system type
AC_CANONICAL_HOST

dnl Put macro definitions here (though they aren't used).
AC_CONFIG_HEADER([src/include/config.h])

dnl Initialise automake stuff.
AM_INIT_AUTOMAKE

dnl Check for iconv
AM_ICONV

dnl Set gettext version
AM_GNU_GETTEXT_VERSION([0.14])

dnl Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_SED
AC_PROG_LN_S

AC_LANG([C++])

AC_PATH_TOOL([STRIP], [strip])
AC_SUBST(STRIP)

PKG_CHECK_EXISTS

dnl ---------------------------------------------------------------------------
dnl Version info
dnl ---------------------------------------------------------------------------

AC_SUBST(PACKAGE_DESCRIPTION,"Server watcher")

app_vrs_major=$(echo $VERSION | cut -d. -f1)
app_vrs_minor=$(echo $VERSION | cut -d. -f2)

AC_DEFINE_UNQUOTED(PACKAGE_MAJOR_VERSION, $app_vrs_major, [The app major version])
AC_DEFINE_UNQUOTED(PACKAGE_MINOR_VERSION, $app_vrs_minor, [The app minor version])

AC_SUBST(PACKAGE_MAJOR_VERSION,$app_vrs_major)
AC_SUBST(PACKAGE_MINOR_VERSION,$app_vrs_minor)

AC_ARG_WITH([release], [AS_HELP_STRING([--with-release], [Set release])], [ app_cv_release="$withval" ],[ app_cv_release="`date +%-y`.`date +%-m`.`date +%-d`" ])

app_rls_major=$(echo $app_cv_release.0.0 | cut -d. -f1)
app_rls_minor=$(echo $app_cv_release.0.0 | cut -d. -f2)

app_vrs_revision=$(date +%Y%m%d)
AC_SUBST(PACKAGE_REVISION,$app_vrs_revision)
AC_DEFINE_UNQUOTED(PACKAGE_REVISION,$app_vrs_revision,[The package revision (for configuration file)])

AC_DEFINE_UNQUOTED(PACKAGE_MAJOR_RELEASE, $app_rls_major, [The library major release])
AC_DEFINE_UNQUOTED(PACKAGE_MINOR_RELEASE, $app_rls_minor, [The library minor release])
AC_DEFINE_UNQUOTED(PACKAGE_RELEASE, "$app_cv_release", [The package release])

AC_SUBST(PACKAGE_RELEASE,"$app_cv_release")
AC_SUBST(PACKAGE_MAJOR_RELEASE,$app_rls_major)
AC_SUBST(PACKAGE_MINOR_RELEASE,$app_rls_minor)

dnl ---------------------------------------------------------------------------
dnl Check for OS specifics
dnl ---------------------------------------------------------------------------

case "$host" in
	*-mingw32|*-pc-msys)
		EXEEXT=".exe"
		OSNAME="windows"

		LIBS="$LIBS -lshell32 -liconv -lws2_32 -lntdll -lwtsapi32 -liphlpapi -lole32 -loleaut32"

		app_win32_revision=$(date +%y.%m.%d.%H)
		AC_SUBST(WIN32_VERSION,$app_win32_revision)

		app_win32_fileversion=$(date +%_y,%_m,%_d,%_H)
		AC_SUBST(WIN32_FILEVERSION,$app_win32_fileversion)

		if test "$host_cpu" = "x86_64"; then
			app_cv_programfiles="PROGRAMFILES64"
			app_cv_winarch="64"
		else
			app_cv_programfiles="PROGRAMFILES"
			app_cv_winarch="32"
		fi

		AC_SUBST(PROGRAMFILES,$app_cv_programfiles)
		AC_SUBST(WINARCH,$app_cv_winarch)

		AC_PATH_TOOL([WINDRES], [windres], [no])
		AC_PATH_TOOL([ZIP], [zip], [no])
		AC_CONFIG_FILES(src/os/windows/resources.rc)
		AC_CONFIG_FILES(win/netwatcher.nsi)
		;;

 	*)
		EXEEXT=""
		OSNAME="linux"
		CPPFLAGS="$CPPFLAGS -pthread"
		LDFLAGS="$LDFLAGS -pthread"

		PKG_CHECK_MODULES( [UDJAT_IPC], [udjat-dbus], AC_DEFINE(HAVE_DBUS,,[Do we have udjat dbus?]), AC_MSG_NOTICE([udjat-dbus not present.]) )


dnl		AC_CONFIG_FILES(sysconf/linux/logrotate)
dnl		AC_CONFIG_FILES(sysconf/linux/permissions)

dnl		AC_CONFIG_FILES(sysconf/linux/systemd.service)
dnl		AC_CONFIG_FILES(sysconf/linux/systemd.preset)

esac

AC_SUBST(DATADIR,$app_cv_datadir)
AC_SUBST(CONFDIR,$app_cv_confdir)
AC_SUBST(LOGDIR,$app_cv_logdir)

AC_SUBST(LIBS)
AC_SUBST(EXEEXT)
AC_SUBST(OSNAME)
AC_SUBST(RLS_LDFLAGS)

AC_SUBST(UDJAT_IPC_LIBS)
AC_SUBST(UDJAT_IPC_CFLAGS)

dnl ---------------------------------------------------------------------------
dnl Check for other programs
dnl ---------------------------------------------------------------------------
AC_PATH_TOOL([VALGRIND], [valgrind], [no])
AC_PATH_TOOL([AR], [ar], [ar])
AC_PATH_TOOL([CONVERT], [convert], [true])

dnl ---------------------------------------------------------------------------
dnl Check for C++ 2017 support
dnl ---------------------------------------------------------------------------
AC_DEFUN([AX_CHECK_COMPILE_FLAG],
[AC_PREREQ(2.59)dnl for _AC_LANG_PREFIX
AS_VAR_PUSHDEF([CACHEVAR],[ax_cv_check_[]_AC_LANG_ABBREV[]flags_$4_$1])dnl
AC_CACHE_CHECK([whether _AC_LANG compiler accepts $1], CACHEVAR, [
  ax_check_save_flags=$[]_AC_LANG_PREFIX[]FLAGS
  _AC_LANG_PREFIX[]FLAGS="$[]_AC_LANG_PREFIX[]FLAGS $4 $1"
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM()],
    [AS_VAR_SET(CACHEVAR,[yes])],
    [AS_VAR_SET(CACHEVAR,[no])])
  _AC_LANG_PREFIX[]FLAGS=$ax_check_save_flags])
AS_IF([test x"AS_VAR_GET(CACHEVAR)" = xyes],
  [m4_default([$2], :)],
  [m4_default([$3], :)])
AS_VAR_POPDEF([CACHEVAR])dnl
])dnl AX_CHECK_COMPILE_FLAGS

AX_CHECK_COMPILE_FLAG([-std=gnu++17], [CPPFLAGS="$CPPFLAGS -std=gnu++17"])

dnl ---------------------------------------------------------------------------
dnl Check for libudjat
dnl ---------------------------------------------------------------------------

PKG_CHECK_MODULES( [UDJAT], [libudjat], AC_DEFINE(HAVE_UDJAT,,[Do we have udjat?]), AC_MSG_ERROR([libudjat not present.]) )
AC_SUBST(UDJAT_LIBS)
AC_SUBST(UDJAT_CFLAGS)

app_cv_product=`$PKG_CONFIG --variable=product_name libudjat`
AC_DEFINE_UNQUOTED(PRODUCT_NAME, $app_cv_product, [The product name])
AC_SUBST(PRODUCT_NAME,$app_cv_product)

app_cv_product_version=`$PKG_CONFIG --modversion libudjat`
AC_DEFINE_UNQUOTED(PRODUCT_VERSION, $app_cv_product_version, [The product name])
AC_SUBST(PRODUCT_VERSION,$app_cv_product_version)

dnl ---------------------------------------------------------------------------
dnl Check for pugixml
dnl ---------------------------------------------------------------------------

PKG_CHECK_MODULES(PUGIXML, pugixml, AC_DEFINE(HAVE_PUGIXML,[],[Do we have PUGIXML?]),AC_MSG_ERROR(pugixml is required))

AC_SUBST(PUGIXML_LIBS)
AC_SUBST(PUGIXML_CFLAGS)

dnl ---------------------------------------------------------------------------
dnl Check for jsoncpp
dnl ---------------------------------------------------------------------------

PKG_CHECK_MODULES( [JSON], [jsoncpp], AC_DEFINE(HAVE_JSONCPP,,[Do we have jsoncpp?]), AC_MSG_ERROR([jsoncpp not present.]) )
AC_SUBST(JSON_LIBS)
AC_SUBST(JSON_CFLAGS)

dnl ---------------------------------------------------------------------------
dnl Output config
dnl ---------------------------------------------------------------------------

AC_SUBST(BASEDIR,$ac_pwd)

dnl ---------------------------------------------------------------------------
dnl Configure which files to generate.
dnl ---------------------------------------------------------------------------

AC_CONFIG_FILES(Makefile)

dnl ---------------------------------------------------------------------------
dnl Output the generated config.status script.
dnl ---------------------------------------------------------------------------
AC_OUTPUT

