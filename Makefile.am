if USING_JINAMP_CTL
bin_PROGRAMS = jinamp jinamp-ctl
else
bin_PROGRAMS = jinamp
endif

bin_SCRIPTS = playaudio
dist_bin_SCRIPTS = playaudio
jinamp_SOURCES = jinamp.c songset.c load.c misc.c options.c debug.c control.c \
                 parse.y conffile.l \
                 songset.h load.h misc.h options.h \
                 parse.h debug.h control.h
jinamp_LDADD = @LIBOBJS@
if USING_JINAMP_CTL
jinamp_ctl_SOURCES = jinamp-ctl.c control.c control.h misc.c misc.h
endif
dist_man_MANS = jinamp.1 jinamp-ctl.1

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

# automake insists that there is a README, so rename it for the distribution
README: README.md
	cp $< $@

CLEANFILES = README
