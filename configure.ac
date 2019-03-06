# For debugging run:
# ./configure CFLAGS="-ggdb3 -O0" CXXFLAGS="-ggdb3 -O0" LDFLAGS="-ggdb3
# otherwise just run:
# ./configure
# Version numbering scheme, see
# https://packaging.python.org/distributing/#choosing-a-versioning-scheme
# 1.2.0.dev1  # Development release
# 1.2.0.a1     # Alpha Release
# 1.2.0.b1     # Beta Release
# 1.2.0.rc1    # Release Candidate
# 1.2.0       # Final Release
# 1.2.0.post1 # Post Release
# 1.2.0.<anything>.s1 # Special of <anything>
AC_INIT([limesdr_test], [0.0.1.dev1], [mats@mcnab.se])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([.])
AM_INIT_AUTOMAKE([foreign subdir-objects])
AC_CONFIG_HEADERS([config.h])
AC_PROG_CXX
AC_PROG_LIBTOOL
AC_SUBST(LIBTOOL_DEPS)
AC_LTDL_DLLIB
AC_CONFIG_FILES([
			Makefile
            src/Makefile
            ])
PKG_CHECK_MODULES([LIMESUITE], [LimeSuite])
CXXFLAGS="$CXXFLAGS \
-std=c++11 \
-pthread \
-Wall \
-Werror \
-Wextra \
-Wredundant-decls \
-Wcast-align \
-Wmissing-declarations \
-Wmissing-include-dirs \
-Winvalid-pch \
-Wredundant-decls \
-Wformat=2 \
-Wmissing-format-attribute \
-Wno-format-nonliteral \
-fvisibility=default \
-lSoapySDR \
-larmadillo "
AC_OUTPUT
