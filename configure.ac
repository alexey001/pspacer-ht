<dnl configure.ac

dnl outline
dnl 1. AC_INIT
dnl 2. options (AC_ARG_ENABLE)
dnl 3. programs (AC_CHECK_PROG)
dnl 4. libraries
dnl 5. header files
dnl 6. typedefs, structs
dnl 7. functions
dnl 8. AC_OUTPUT

###
### 1. AC_INIT
###

AC_INIT([pspacer-ht],[ ],[takano-ryousei@aist.go.jp])
PACKAGE_VERSION=`cat $srcdir/VERSION | awk '{print $1}'`
PACKAGE_STRING="$PACKAGE_NAME-$PACKAGE_VERSION"

AC_SUBST(RELEASE_DATE)
RELEASE_DATE=`cat $srcdir/VERSION | awk '{print $2}'`

arch=`uname -i`
if test "$arch" == "x86_64" ; then
    tclib_dir="/usr/lib64/tc"
else
    tclib_dir="/usr/lib/tc"
fi

function fullpath {
    (cd `dirname $1`; pwd) \
    | tr -d "\n"; echo /`basename $1`
}

###
### 2. options (AC_ARG_ENABLE)
###

dnl CFLAGS

AC_SUBST(DEBUG_COPT)

AC_ARG_ENABLE(debug,
    AS_HELP_STRING([--enable-debug],[debugging mode]),
    [],enable_debug=no)
case "${enable_debug}" in
yes)
    DEBUG_COPT="-DDEBUG=1"
    ;;
*) ;;
esac

dnl iproute2

AC_SUBST(IPROUTE_INC)
AC_SUBST(TCLIB_DIR)

AC_ARG_WITH(iproute2_dir,
    AS_HELP_STRING([--with-iproute2-dir=DIR],
    [specify the iproute2 directory (default /opt/iproute2)]),
    [], [with_iproute2_dir=/opt/iproute2])

AC_ARG_WITH(tclib_dir,
    AS_HELP_STRING([--with-tclib-dir=DIR],
    [specify the tc lib directory (default /usr/lib/tc for i368, /usr/lib64/tc for x86_64)]),
     [], [with_tclib_dir=${tclib_dir}])

abs_iproute2_dir=""
case ${with_iproute2_dir} in
    /*) # absolute path
	abs_iproute2_dir=${with_iproute2_dir}
	;;
    *) # relative path
	abs_iproute2_dir=`fullpath ${with_iproute2_dir}`
	;;
esac

if test ! -d ${with_iproute2_dir}; then
    # For checking Debian iproute-dev package.
    if test -d /usr/include/iproute; then
        IPROUTE_INC="-I /usr/include/iproute"
    else
        echo "${with_iproute2_dir} does not exist."
        exit 1
    fi
else
    IPROUTE_INC="-I ${abs_iproute2_dir}/include -I ${abs_iproute2_dir}/tc"
fi

TCLIB_DIR=${with_tclib_dir}

###
### 3. programs (AC_CHECK_PROG)
###

AC_PROG_CC
AC_PROG_INSTALL

###
### 4. libraries
###

###
### 5. header files
###

###
### 6. typedefs, structs
###

###
### 7. functions
###

###
### 8. AC_OUTPUT
###

echo ""
echo "Configuration"
(
echo "  --enable-debug                  ${enable_debug}"
echo "  --with-iproute2-dir             ${with_iproute2_dir}"
echo "  --with-tclib-dir                ${with_tclib_dir}"
) | tee config.options
echo ""

AC_CONFIG_FILES([Makefile
	kernel/Makefile
	tc/Makefile])
AC_OUTPUT

