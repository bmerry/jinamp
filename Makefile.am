bin_PROGRAMS = jinamp
bin_SCRIPTS = playaudio
dist_bin_SCRIPTS = playaudio
jinamp_SOURCES = jinamp.c list.c load.c misc.c options.c debug.c \
                 parse.y conffile.l list.h load.h misc.h \
                 options.h parse.h debug.h
man1_MANS = jinamp.1.gz
EXTRA_DIST = jinamp.1

CLEANFILES = jinamp.1.gz
MAINTAINERCLEANFILES = parse.c conffile.c

jinamp.1.gz: jinamp.1
	gzip -9 -c jinamp.1 > jinamp.1.gz

AM_YFLAGS = -d
conffile.o options.o: parse.h

if DEBUG
extra_cflags = -DDEBUG=1
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
