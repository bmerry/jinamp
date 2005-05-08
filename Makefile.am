if USING_JINAMP_CTL
bin_PROGRAMS = jinamp jinamp-ctl
else
bin_PROGRAMS = jinamp
endif

bin_SCRIPTS = playaudio
dist_bin_SCRIPTS = playaudio
jinamp_SOURCES = jinamp.c set.c load.c misc.c options.c debug.c control.c \
                 songring.c parse.y conffile.l \
                 set.h songring.h load.h misc.h options.h \
                 parse.h debug.h control.h
jinamp_LDADD = @LIBOBJS@
if USING_JINAMP_CTL
jinamp_ctl_SOURCES = jinamp-ctl.c control.c control.h misc.c misc.h
endif
dist_man_MANS = jinamp.1 jinamp-ctl.1
EXTRA_DIST = debian/rules debian/control debian/copyright debian/changelog debian/docs debian/dirs

MAINTAINERCLEANFILES = parse.c parse.h conffile.c

AM_YFLAGS = -d
conffile.o options.o: parse.h

AM_CFLAGS = -DDEFAULT_PLAYER=\"@DEFAULT_PLAYER@\"
if DEBUG
AM_CFLAGS += -DDEBUG=1 -g -Wall
endif

if USE_SUID
install-exec-hook:
	chmod u+s ${DESTDIR}${bindir}/jinamp
else
install-exec-hook:
endif
