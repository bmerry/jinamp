bin_PROGRAMS = jinamp jinamp-ctl
bin_SCRIPTS = playaudio
dist_bin_SCRIPTS = playaudio
jinamp_SOURCES = jinamp.c list.c load.c misc.c options.c debug.c control.c \
                 parse.y conffile.l list.h load.h misc.h \
                 options.h parse.h debug.h control.h
jinamp_LDADD = @LIBOBJS@
jinamp_ctl_SOURCES = jinamp-ctl.c control.c control.h misc.c misc.h
dist_man_MANS = jinamp.1 jinamp-ctl.1
EXTRA_DIST = debian/rules debian/control debian/copyright debian/changelog debian/docs debian/dirs

MAINTAINERCLEANFILES = parse.c parse.h conffile.c

AM_YFLAGS = -d
conffile.o options.o: parse.h

if DEBUG
extra_cflags = -DDEBUG=1 -g -Wall
else
extra_cflags =
endif

AM_CFLAGS = -DDEFAULT_PLAYER=\"@DEFAULT_PLAYER@\" $(extra_cflags)

if USE_SUID
install-exec-hook:
	chmod u+s ${DESTDIR}${bindir}/jinamp
else
install-exec-hook:
endif
