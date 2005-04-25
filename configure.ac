AC_INIT(jinamp, 1.1, bmerry@users.sourceforge.net)
AC_CONFIG_SRCDIR(jinamp.c)
AM_INIT_AUTOMAKE
AM_CONFIG_HEADER(config.h)

dnl Checks for programs
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_AWK
AM_PROG_LEX
AC_PROG_YACC

dnl Checks for libraries

dnl Checks for header files
AC_HEADER_STDC
if [[ "$ac_cv_header_stdc" = no ]]
then
  AC_MSG_ERROR([[A system with standard ANSI C headers is required]])
fi
AC_HEADER_SYS_WAIT
AC_HEADER_DIRENT
AC_CHECK_HEADERS([sys/resource.h limits.h regex.h sys/ipc.h sys/msg.h])
AC_CHECK_HEADERS([fcntl.h], [], [AC_MSG_ERROR([[Missing fcntl.h]])])
AC_CHECK_HEADERS([stddef.h], [], [AC_MSG_ERROR([[Missing stddef.h]])])

dnl Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_C_CONST

dnl Checks for library functions.
AC_FUNC_FORK
if [[ "$ac_cv_func_fork_works" = no ]]
then
  AC_MSG_ERROR([[A working version of fork() is required]])
fi
AC_FUNC_VPRINTF
AC_TYPE_SIGNAL
AC_REPLACE_FUNCS([memset])
AC_CHECK_FUNCS([setpriority pathconf strtol realpath getopt_long regcomp atexit dup2])
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

have_ipc=no
if test x$ac_cv_header_sys_ipc_h = xyes
then
  if test x$ac_cv_header_sys_msg_h = xyes
  then
    have_ipc=yes
  fi
fi
AM_CONDITIONAL(USING_JINAMP_CTL, test x$have_ipc = xyes)

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
if test x$ac_cv_func_getopt_long = xno
then
  AC_MSG_WARN([[getopt_long not found, so long options won't work]])
fi
if test x$ac_cv_func_regcomp = xno
then
  AC_MSG_WARN([[regcomp not found, so playlists and ignore sets won't work]])
fi
if test x$have_ipc = xno
then
  AC_MSG_WARN([[System V IPC not found, so jinamp-ctl will not be built]])
fi
