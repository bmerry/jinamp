AC_INIT(jinamp, 0.3.1, bmerry@sourceforge.net)
AC_CONFIG_SRCDIR(jinamp.c)
AM_INIT_AUTOMAKE(jinamp, 0.3.1)
AM_CONFIG_HEADER(config.h)

dnl Checks for programs
AC_PROG_CC
AC_PROG_INSTALL
AM_PROG_LEX
AC_PROG_YACC

dnl Checks for libraries

dnl Checks for header files
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_HEADER_DIRENT
AC_CHECK_HEADERS([sys/resource.h limits.h regex.h])

dnl Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_C_CONST

dnl Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_VPRINTF
AC_TYPE_SIGNAL
AC_FUNC_STAT
AC_CHECK_FUNCS([setpriority pathconf strtol realpath getopt_long regcomp])
dnl --enable options
debug=false
AC_ARG_ENABLE(debug, [  --enable-debug          enable debugging code (quite verbose) [[no]]], [
case "${enableval}" in
  yes) debug=true ;;
  no) debug=false ;;
  *) AC_MSG_ERROR([bad value ${enableval} for --enable-debug]) ;;
esac], [debug=false])
AM_CONDITIONAL(DEBUG, test x$debug = xtrue)

use_suid=false
AC_ARG_ENABLE(suid, [  --enable-suid           install SUID root and use high priority scheduling [[no]]], [
case "${enableval}" in
  yes) use_suid=true ;;
  no) use_suid=false ;;
  *) AC_MSG_ERROR([bad value ${enableval} for --enable-suid]) ;;
esac], [use_suid=false])
AM_CONDITIONAL(USE_SUID, test x$use_suid = xtrue)

dnl --with options
DEFAULT_PLAYER='${bindir}/playaudio'
AC_ARG_WITH(player, [  --with-player=player    use `player' as the default player],
[
case "${withval}" in
  yes|no) AC_MSG_ERROR([--with-player must take an argument]) ;;
  *) DEFAULT_PLAYER="${withval}" ;;
esac], [])
AC_SUBST(DEFAULT_PLAYER)

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
if [[ "$ac_cv_func_getopt_long" = no ]]
then
  AC_MSG_WARN([getopt_long not found, so long options won't work])
fi
if [[ "$ac_cv_func_regcomp" = no ]]
then
  AC_MSG_WARN([regcomp not found, so playlists and ignore sets won't work])
fi
