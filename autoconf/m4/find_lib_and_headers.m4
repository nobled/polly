dnl find_lib_and_headers(name, verify-header, library-name, requirded?)
dnl Export
dnl         name_inc in -I"include-path" form
dnl         name_lib in -l"library-name" form
dnl         name_ld  in -L"library-path" form
dnl         name_found set to "yes" if found

AC_DEFUN([find_lib_and_headers],
[
  AC_LANG_PUSH(C++)
  OLD_CXXFLAGS=$CXXFLAGS;
  OLD_LDFLAGS=$LDFLAGS;
  OLD_LIBS=$LIBS;

  LIBS="$LIBS -l$3";

  # Get include path
  AC_ARG_WITH([$1-include-path],
    [AS_HELP_STRING([--with-$1-include-path], [location of $1 includes])],
      [given_inc_path=$withval; CXXFLAGS="-I$withval $CXXFLAGS"],
      [given_inc_path=inc_not_give_$1]
    )
  # Get lib path
  AC_ARG_WITH([$1-library-path],
    [AS_HELP_STRING([--with-$1-library-path],[location of the $1 libraries])],
      [given_lib_path=$withval; LDFLAGS="-L$withval $LDFLAGS"],
      [given_lib_path=lib_not_give_$1]
  )
  # Check for library and headers works
  AC_MSG_CHECKING([for $1])
  # try to compile a file that includes a header of the library
  AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <$2>]], [[;]])],
    [AC_MSG_RESULT([ok])
    AC_SUBST([$1_found],["yes"])
    AS_IF([test "x$given_inc_path" != "xinc_not_give_$1"],
      [AC_SUBST([$1_inc],["-I$given_inc_path"])])
    AC_SUBST([$1_lib],["-l$3"])
    AS_IF([test "x$given_lib_path" != "xlib_not_give_$1"],
      [AC_SUBST([$1_ld],["-L$given_lib_path"])])],
    [AS_IF([test "x$4" = "xrequired"],
      [AC_MSG_ERROR([$1 required but not found])],
      [AC_MSG_RESULT([not found])])]
  )

  # reset original CXXFLAGS
  CXXFLAGS=$OLD_CXXFLAGS
  LDFLAGS=$OLD_LDFLAGS;
  LIBS=$OLD_LIBS
  AC_LANG_POP(C++)
])
