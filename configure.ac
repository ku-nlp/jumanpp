#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])

AC_INIT([JUMAN++], [1.01], [morita@nlp.ist.i.kyoto-u.ac.jp],[jumanpp])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([src/morph.cc])
AC_CONFIG_HEADERS([src/config.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_RANLIB

AC_LANG_PUSH(C++)
AX_CXX_COMPILE_STDCXX_11([noext], [mandatory])
#AX_CXX_COMPILE_STDCXX_14([noext], [mandatory]) # This macro requires autotools archive 2016-03-20 or newer.
AX_BOOST_BASE([1.57], boost_check=0, boost_check=1)

if test $boost_check = 1; then
    AC_MSG_ERROR("Error: cannot find available Boost library.")
fi
AC_CHECK_LIB([tcmalloc], [main], LIBS="-ltcmalloc $LIBS" has_libtcmalloc=1, has_libtcmalloc=0)
if test $has_libtcmalloc = 0; then
    AC_MSG_WARN("Warning: cannot find -ltcmalloc. It is recommended to link libtcmalloc.")
fi
AC_LANG_POP(C++)

AC_CHECK_LIB([m], [main])
AC_CHECK_LIB([pthread], [main])
# Check for Mac
AC_CHECK_LIB([rt], [main], LIBS="-lrt $LIBS" has_librt=1,has_librt=0)

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h float.h inttypes.h malloc.h memory.h shadow.h stddef.h stdlib.h string.h sys/file.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_UID_T
AC_C_INLINE
AC_TYPE_INT64_T
AC_TYPE_SIZE_T
AC_TYPE_UINT64_T

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_FUNC_REALLOC
AC_CHECK_FUNCS([memmove memset munmap setenv sqrt strchr strerror strrchr strstr strtol strtoul])

LIBS="$LIBS cdb/libcdb.a"

# Set Default prefix
test "${prefix}" = NONE && prefix=/usr/local

RESOURCE_DIR="${datadir}/jumanpp"
eval RESOURCE_DIR=`eval echo "$RESOURCE_DIR"`

# Add option for resource path
AC_ARG_ENABLE([default_resource_path],
	[AS_HELP_STRING([--enable-default-resource-path=PATH], [set default resource path])],
    [DEFAULT_RESOURCE_PATH=${enableval}],
    [DEFAULT_RESOURCE_PATH=${RESOURCE_DIR}]
)

AS_IF([test "x${DEFAULT_MODEL_PATH}" != "xdisable"],
    [AC_DEFINE_UNQUOTED(DEFAULT_RESOURCE_PATH, ${DEFAULT_RESOURCE_PATH}, [default path to resource])]
)

AC_ARG_ENABLE([no_dict],
    [AS_HELP_STRING([--enable-no-dict], [do not install resource files ])],
    [NODICT_OPTION="true"],
    [NODICT_OPTION="false"]
)
AM_CONDITIONAL(NODICT, test x${NODICT_OPTION} = xtrue)

AC_ARG_ENABLE([dev_option],
	[AS_HELP_STRING([--enable-dev_option], [enable options for development ])],
    [DEV_OPTION=${enableval}],
    [DEV_OPTION="disable"]
)

AS_IF([test "x${DEV_OPTION}" != "xdisable"],
    [AC_DEFINE_UNQUOTED(USE_DEV_OPTION, ${DEV_OPTION}, [enables options for development])]
)

AS_IF([test -e .git],
    [GIT_VER_TMP=`git log -1 --date=local --format="%ad %h"`;echo ${GIT_VER_TMP} > VERSION],
    [GIT_VER_TMP=]
)
AC_DEFINE_UNQUOTED(GITVER, "${GIT_VER_TMP}", [revision])
      
AC_CONFIG_FILES([Makefile src/Makefile src/cdb/Makefile])
AC_OUTPUT

