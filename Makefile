
CC = gcc

CTFMERGE = /bin/true
CTFCONVERT = /bin/true

UNAME_S := $(shell uname -s)
PLATFORM_OK = false

CFILES = dynstr.c proto.c common.c base64.c crc32.c reqid.c
OBJS = $(CFILES:%.c=%.o)
HDRS = dynstr.h plat.h proto.h common.h base64.h crc32.h reqid.h
CFLAGS = -I$(PWD) -Wall -Wextra -Werror -g -O2
LDLIBS =

BINDIR = /usr/sbin
DESTDIR = $(PWD)/proto

PROGS = \
	mdata-get \
	mdata-list \
	mdata-put \
	mdata-delete

PROTO_PROGS = \
	$(PROGS:%=$(DESTDIR)$(BINDIR)/%)


#
# Platform-specific definitions
#

ifeq ($(UNAME_S),SunOS)
CFLAGS += -D__HAVE_BOOLEAN_T
CFILES += plat/sunos.c plat/unix_common.c
HDRS += plat/unix_common.h
LDLIBS += -lnsl -lsocket -lsmbios
PLATFORM_OK = true
endif

ifeq ($(UNAME_S),Linux)
CFILES += plat/linux.c plat/unix_common.c
HDRS += plat/unix_common.h
PLATFORM_OK = true
endif

ifeq ($(PLATFORM_OK),false)
$(error Unknown platform: $(UNAME_S))
endif

#
# Build Targets
#

.PHONY:	all world
world:	all
all:	$(PROGS)

%.o:	%.c
	$(CC) -c $(CFLAGS) -o $@ $<
	$(CTFCONVERT) -l mdata-client $@

mdata-%:	$(OBJS) $(HDRS) mdata_%.o
	$(CC) $(CFLAGS) $(LDLIBS) -o $@ $(@:mdata-%=mdata_%).o $(OBJS)
	$(CTFMERGE) -l mdata-client -o $@ $(OBJS) $(@:mdata-%=mdata_%).o

#
# Install Targets
#

.PHONY:	install
install:	$(PROTO_PROGS)

$(DESTDIR)$(BINDIR)/%: %
	@mkdir -p $(DESTDIR)$(BINDIR)
	cp $< $@
	touch $@

#
# SmartOS (smartos-live) Package Manifest Targets
#

.PHONY: manifest
manifest:
	cp manifest $(DESTDIR)/$(DESTNAME)

#
# Cleanup Targets
#

.PHONY:	clean
clean:
	rm -f $(PROGS) $(OBJS)

.PHONY:	clobber
clobber:	clean
	rm -rf $(PWD)/proto
