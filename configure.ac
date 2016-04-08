#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([JUMAN++], [0.95], [hmorita@i.kyoto-u.ac.jp])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([src/morph.cc])
#AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_RANLIB

# Checks for libraries.
AC_MSG_CHECKING([for option --std=c++1y])
CXXFLAGS="$CXXFLAGS -std=c++1y"
LIBS="$LIBS -lstdc++"
AC_LANG_PUSH(C++)
AC_TRY_LINK([], 
            [],
            has_cpp14_option=1,
            has_cpp14_option=0)
if test $has_cpp14_option = 0; then
    AC_MSG_RESULT([no])
    AC_MSG_ERROR("ERROR: --std=c++1y option does not supported.")
else
    AC_MSG_RESULT([yes])
fi

# Checks for libraries.
AC_MSG_CHECKING([for -lboost_serialization])
AC_LANG_PUSH(C++)
LIBS="$LIBS -lboost_serialization"
AC_TRY_LINK([#include <fstream>
             #include <boost/serialization/unordered_map.hpp>
             #include <boost/archive/binary_iarchive.hpp>
             #include <boost/archive/binary_oarchive.hpp>
             #include <boost/archive/impl/basic_binary_oarchive.ipp>
             #include <boost/archive/impl/basic_binary_iarchive.ipp>], 
            [std::ofstream out("test");
             boost::archive::binary_oarchive object(out);],
            has_boost_serialization_lib=1,
            has_boost_serialization_lib=0)
if test $has_boost_serialization_lib = 0; then
    AC_MSG_RESULT([no])
    AC_MSG_ERROR("ERROR: cannot find -lboost_serialization.")
else
    AC_MSG_RESULT([yes])
fi

AC_CHECK_LIB([tcmalloc], [main], LIBS="-ltcmalloc $LIBS" has_libtcmalloc=1, has_libtcmalloc=0)
if test $has_libtcmalloc = 0; then
    AC_MSG_WARN("Warning: cannot find -ltcmalloc. It is recommended to link libtcmalloc.")
fi
AC_LANG_POP(C++)

#AC_CHECK_LIB([boost_serialization], [main])
AC_CHECK_LIB([m], [main])
AC_CHECK_LIB([pthread], [main])
# Check for Mac
AC_CHECK_LIB([rt], [main], LIBS="-lrt $LIBS" has_librt=1,has_librt=0)

cd src/cdb && make && cd -
LIBS="$LIBS src/cdb/libcdb.a"

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

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
