dnl configure.ac
dnl
dnl xfce4-squeezebox-plugin - squeezebox plugin for the XFce4 panel
dnl

m4_define([test_version], [0.1.0])

AC_INIT([xfce4-squeezebox-plugin], [test_version()],
	[hakan@erduman.de])


TEST_VERSION=test_version()
AM_INIT_AUTOMAKE([xfce4-squeezebox-plugin], [$TEST_VERSION])
AC_CONFIG_HEADER([config.h])

AM_MAINTAINER_MODE

dnl Check for UNIX variants
AC_AIX
AC_ISC_POSIX
AC_MINIX

dnl Check for basic programs
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_INTLTOOL
AC_PROG_LIBTOOL

dnl Check for standard header files
AC_HEADER_STDC

dnl configure the panel plugin
XDT_CHECK_PACKAGE([LIBXFCE4PANEL], [libxfce4panel-1.0], [4.3.99])

dnl check for dbus for rhythmbox 0.9.5
XDT_CHECK_PACKAGE([DBUSGLIB], [dbus-glib-1], [0.60])
AC_ARG_ENABLE(DBUS,[  --disable-DBUS     disable DBUS backends support (default: enable)],[enable_DBUS=$enableval],[enable_DBUS=yes])
if test x$enable_DBUS = xyes; then
	AC_DEFINE(HAVE_DBUS,1,[Define to use DBUS backends])
fi

GETTEXT_PACKAGE=xfce4-squeezebox-plugin
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [Define to the Gettext package name])

AM_GLIB_GNU_GETTEXT

dnl check for exo presence
PKG_CHECK_MODULES([EXO], [exo-0.3 >= 0.3.0])

dnl check for new or old tooltips
PKG_CHECK_MODULES(GTK,                            \
    gtk+-2.0 >= 2.12.0,
    have_newtooltips=yes,
    have_newtooltips=no)
if test x$have_newtooltips = xyes ; then
    AC_DEFINE(HAVE_GTK_2_12, 1, [Define if GtkTooltips is deprecated])
fi

dnl check for libnotify, taken from rhythmbox 0.9.5
LIBNOTIFY_REQUIRED=0.2.2
AC_ARG_ENABLE(libnotify,
            AC_HELP_STRING([--disable-libnotify],
			   [Disable libnotify support]),,
	      enable_libnotify=auto)
if test "x$enable_libnotify" != "xno"; then
	PKG_CHECK_MODULES(NOTIFY,                            \
			  libnotify >= $LIBNOTIFY_REQUIRED,
			  have_libnotify=yes,
			  have_libnotify=no)
	if test "x$have_libnotify" = "xno" -a "x$enable_libnotify" = "xyes"; then
	  AC_MSG_ERROR([libnotify support explicitly requested, but libnotify couldn't be found])
	fi
	if test "x$have_libnotify" = "xyes"; then
	     enable_libnotify=yes
	fi
fi			  
AM_CONDITIONAL(USE_NOTIFY, test x"$enable_libnotify" = xyes)
if test x$enable_libnotify = xyes ; then
    # Find out the version of LIBNOTIFY we're using
    libnotify_version=`pkg-config --modversion libnotify`
    LIBNOTIFY_VERSION_MAJOR=`echo $libnotify_version | awk -F. '{print $1}'`
    LIBNOTIFY_VERSION_MINOR=`echo $libnotify_version | awk -F. '{print $2}'`
    LIBNOTIFY_VERSION_MICRO=`echo $libnotify_version | awk -F. '{print $3}'`
    if test "z$LIBNOTIFY_VERSION_MAJOR" = "z"; then
        LIBNOTIFY_VERSION_MAJOR="0"
    fi
    if test "z$LIBNOTIFY_VERSION_MINOR" = "z"; then
        LIBNOTIFY_VERSION_MINOR="0"
    fi
    if test "z$LIBNOTIFY_VERSION_MICRO" = "z"; then
        LIBNOTIFY_VERSION_MICRO="0"
    fi
    echo "Your libnotify version is $LIBNOTIFY_VERSION_MAJOR,$LIBNOTIFY_VERSION_MINOR,$LIBNOTIFY_VERSION_MICRO."
    NOTIFY_CFLAGS="$NOTIFY_CFLAGS -DLIBNOTIFY_VERSION_MAJOR=$LIBNOTIFY_VERSION_MAJOR"
    NOTIFY_CFLAGS="$NOTIFY_CFLAGS -DLIBNOTIFY_VERSION_MINOR=$LIBNOTIFY_VERSION_MINOR"
    NOTIFY_CFLAGS="$NOTIFY_CFLAGS -DLIBNOTIFY_VERSION_MICRO=$LIBNOTIFY_VERSION_MICRO"

    AC_DEFINE(HAVE_NOTIFY, 1, [Define if libnotify support is enabled])
    AC_SUBST(NOTIFY_CFLAGS)
    AC_SUBST(NOTIFY_LIBS)
fi

dnl in case we should ever need our own mp3 tag lookup, use this and make it default
dnl AC_ARG_ENABLE(id3,[  --disable-id3           disable id3 support (default: enable)],[enable_id3=$enableval],[enable_id3=yes])
AC_ARG_ENABLE(id3,[  --disable-id3           disable id3 support (default: disable)],[enable_id3=$enableval],[enable_id3=no])

AC_ARG_WITH(id3tag,[  --with-id3tag=PFX           Prefix where libid3tag is installed (optional)], id3tag_prefix="$withval", id3tag_prefix="")
AC_ARG_WITH(id3tag-libraries,[  --with-id3tag-libraries=DIR Directory where libid3tag library is installed (optional)], id3tag_libraries="$withval", id3tag_libraries="")
AC_ARG_WITH(id3tag-includes,[  --with-id3tag-includes=DIR  Directory where libid3tag header files are installed (optional)], id3tag_includes="$withval", id3tag_includes="")

ID3_SUBDIR=""

if test x$enable_id3 = xyes; then
	if test "x$id3tag_libraries" != "x" ; then
		ID3TAG_LIBS="-L$id3tag_libraries"
	elif test "x$id3tag_prefix" != "x" ; then
		ID3TAG_LIBS="-L$id3tag_prefix/lib"
	elif test "x$prefix" != "xNONE"; then
		ID3TAG_LIBS="-L$libdir"
	fi

	ID3TAG_LIBS="$ID3TAG_LIBS -lid3tag -lz"

	if test "x$id3tag_includes" != "x" ; then
		ID3TAG_CFLAGS="-I$id3tag_includes"
	elif test "x$id3tag_prefix" != "x" ; then
		ID3TAG_CFLAGS="-I$id3tag_prefix/include"
	elif test "x$prefix" != "xNONE"; then
		ID3TAG_CFLAGS="-I$includedir"
	fi

	ID3TAG_CFLAGS="$ID3TAG_CFLAGS"

	oldcflags=$CFLAGS
	oldlibs=$LIBS
	oldcppflags=$CPPFLAGS
	CFLAGS="$CFLAGS $MPD_CFLAGS $ID3TAG_CFLAGS"
	LIBS="$LIBS $MPD_LIBS $ID3TAG_LIBS"
	CPPFLAGS=$CFLAGS
	AC_CHECK_HEADERS(id3tag.h,use_libid3tag=yes,
		[use_libid3tag=no])
	if test x$use_libid3tag = xyes; then
		AC_CHECK_LIB(id3tag,id3_file_open,
			[LIBMPD_LIBS="$LIBMPD_LIBS $ID3TAG_LIBS";
			LIBMPD_CFLAGS="$LIBMPD_CFLAGS $ID3TAG_CFLAGS";
			use_libid3tag=yes],
			[use_libid3tag=no])
	fi
	CFLAGS=$oldcflags
	LIBS=$oldlibs
	CPPFLAGS=$oldcppflags
	if test x$use_libid3tag = xyes; then
		AC_DEFINE(HAVE_ID3TAG,1,[Define to use id3tag])
	else
		enable_id3=no
	fi
fi

dnl Add backends here
AC_ARG_ENABLE(rhythmbox,[  --disable-rhythmbox     disable rhythmbox support (default: enable)],[enable_rhythmbox=$enableval],[enable_rhythmbox=yes])
if test x$enable_rhythmbox = xyes && test x$enable_DBUS = xyes; then
	AC_DEFINE(HAVE_BACKEND_RHYTHMBOX,1,[Define to use rhythmbox])
fi

AC_ARG_ENABLE(MPD,[  --disable-MPD           disable MPD support (default: enable)],[enable_MPD=$enableval],[enable_MPD=yes])
if test x$enable_MPD = xyes; then
	AC_DEFINE(HAVE_BACKEND_MPD,1,[Define to use MPD])

	dnl check for libmpd presence
	AC_ARG_WITH(libmpd,
		AC_HELP_STRING([--with-libmpd=path], [path to libmpd directory]),
			LIBMPD_PATH="$withval", LIBMPD_PATH="")

	AC_MSG_CHECKING(whether --libmpd was used)
	if test -n "$LIBMPD_PATH" ; then
	   AC_MSG_RESULT(yes, using $LIBMPD_PATH)
	   LIBMPD_CFLAGS="-I${LIBMPD_PATH}/include/"
	   LIBMPD_LIBS="-L${LIBMPD_PATH}/lib/ -lmpd"
	else
	   AC_MSG_RESULT(no, falling back to pkg-config)
	   PKG_CHECK_MODULES([LIBMPD], [libmpd >= 0.12])
	fi
	AC_SUBST([LIBMPD_CFLAGS])
	AC_SUBST([LIBMPD_LIBS])
fi

AC_ARG_ENABLE(quodlibet,[  --disable-quodlibet           disable QuodLibet support (default: enable)],[enable_quodlibet=$enableval],[enable_quodlibet=yes])
if test x$enable_quodlibet = xyes; then
	AC_DEFINE(HAVE_BACKEND_QUODLIBET,1,[Define to use QL])
	dnl Checking for thunar-vfs
	XDT_CHECK_PACKAGE([THUNARVFS], [thunar-vfs-1], [0.4.0])
	AC_SUBST([THUNARVFS_CFLAGS])
	AC_SUBST([THUNARVFS_LIBS])
	dnl Checking for libwnck
	XDT_CHECK_PACKAGE([LIBWNCK], [libwnck-1.0], [2.14.3])
	AC_SUBST([LIBWNCK_CFLAGS])
	AC_SUBST([LIBWNCK_LIBS])
fi

AC_ARG_ENABLE(audacious,[  --disable-audacious     disable audacious support (default: enable)],[enable_audacious=$enableval],[enable_audacious=yes])
if test x$enable_audacious = xyes && test x$enable_DBUS = xyes; then
	AC_DEFINE(HAVE_BACKEND_AUDACIOUS,1,[Define to use audacious])
fi

AC_ARG_ENABLE(exaile,[  --disable-exaile     disable exaile support (default: enable)],[enable_exaile=$enableval],[enable_exaile=yes])
if test x$enable_exaile = xyes && test x$enable_DBUS = xyes; then
	AC_DEFINE(HAVE_BACKEND_EXAILE,1,[Define to use exaile])
fi

AC_ARG_ENABLE(consonance,[  --disable-consonance     disable consonance support (default: enable)],[enable_consonance=$enableval],[enable_consonance=yes])
if test x$enable_consonance = xyes && test x$enable_DBUS = xyes; then
	AC_DEFINE(HAVE_BACKEND_CONSONANCE,1,[Define to use consonance])
fi

dnl Translations
XDT_I18N([de fr tr])

dnl Check for debugging support
XDT_FEATURE_DEBUG()
AC_ARG_ENABLE(debug_trace,[  --disable-debug_trace     disable trace support (default: enable)],[enable_debug_trace=$enableval],[enable_debug_trace=yes])
if test x$enable_debug_trace = xyes; then
    AC_DEFINE([DEBUG_TRACE], [1], [Set this to 1 to enable full trace debugging.])
fi

dnl PKG_CHECK_MODULES(LIBGTK, gtk+ libxfce4panel-1.0)
AC_SUBST(LIBGTK_CFLAGS)
AC_SUBST(LIBGTK_LIBS)
AM_PATH_GLIB_2_0(2.12.0,,AC_MSG_ERROR([
*** GLib 2.12 is required to build xfce4-squeezebox-plugin; please make sure you have the
*** GLib development headers installed. The latest version of GLib is always
*** available at http://www.gtk.org/.]))
AC_SUBST(GLIB_LIBS)
AC_SUBST(GLIB_CFLAGS)

AC_OUTPUT([
Makefile
panel-plugin/Makefile
po/Makefile.in
])

echo ""
echo "########### SqueezeBox CONFIGURATION ############"
echo ""

if test x$enable_DBUS = xyes; then
	echo " DBUS support ..................enabled"
else
	echo " DBUS support ..................disabled"
fi

if test x$enable_id3 = xyes; then
	echo " ID3 tag support ...............enabled"
else
	echo " ID3 tag support ...............disabled"
fi

if test x$enable_libnotify = xyes; then
	echo " Notify support ................enabled"
else
	echo " Notify support ................disabled"
fi

if test x$have_newtooltips = xyes ; then
	echo " Tooltips API ..................new"
else
	echo " Tooltips API ..................old"
fi

echo ""
echo "----------- SqueezeBox Backends -----------------"
echo ""

if test x$enable_rhythmbox = xyes && test x$enable_DBUS = xyes; then
	echo " rhythmbox support .............enabled"
else
	echo " rhythmbox support .............disabled"
fi

if test x$enable_MPD = xyes; then
	echo " MPD support ...................enabled"
else
	echo " MPD support ...................disabled"
fi

if test x$enable_QUODLIBET = xyes; then
	echo " QuodLibet support .............enabled"
else
	echo " QuodLibet support ..... .......disabled"
fi

if test x$enable_audacious = xyes && test x$enable_DBUS = xyes; then
	echo " audacious support .............enabled"
else
	echo " audacious support .............disabled"
fi

if test x$enable_exaile = xyes && test x$enable_DBUS = xyes; then
	echo " exaile support ................enabled"
else
	echo " exaile support ................disabled"
fi

if test x$enable_consonance = xyes && test x$enable_DBUS = xyes; then
	echo " consonance support ............enabled"
else
	echo " consonance support ............disabled"
fi

echo ""
