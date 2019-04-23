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
AC_INIT([limesdr_test], [0.0.9.dev1], [mats@mcnab.se])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([.])
AM_INIT_AUTOMAKE([foreign subdir-objects])
AC_CONFIG_HEADERS([config.h])
AC_PROG_CXX
AC_PROG_LIBTOOL
AC_SUBST(LIBTOOL_DEPS)
AC_LTDL_DLLIB
AC_CHECK_PROGS([DOXYGEN], [doxygen])
if test -z "$DOXYGEN"; then
AC_MSG_WARN([Doxygen not found - continue without Doxygen support])
fi
AC_CHECK_PROGS([DOT], [dot])
if test -z "$DOT"; then
AC_MSG_ERROR([Doxygen needs dot, please install dot first (On Arch install graphviz)])
fi
AC_CHECK_PROGS([PDFLATEX], [pdflatex])
if test -z "$PDFLATEX"; then
AC_MSG_ERROR([Doxygen needs pdflatex program, it is part of TeX http://www.tug.org/texlive/acquire-netinstall.html])
fi
AC_CONFIG_FILES([
			Makefile
            src/Makefile
            include/Makefile
            include/tclap/Makefile
            ])
AM_CONDITIONAL([HAVE_DOXYGEN], [test -n "$DOXYGEN"])
AM_COND_IF([HAVE_DOXYGEN], [AC_CONFIG_FILES([docs/Doxyfile])])
AM_COND_IF([HAVE_DOXYGEN], [AC_CONFIG_FILES([docs/Makefile])])
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
-larmadillo \
-lstdc++ \
-lLimeSuite "
AC_OUTPUT
