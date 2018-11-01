
VERSION = 0.0.1

CC = gcc

CTFMERGE = /bin/true
CTFCONVERT = /bin/true

GNUTAR = tar

PWD := $(shell pwd)
UNAME_S := $(shell uname -s)
PLATFORM_OK = false

CFILES = dynstr.c proto.c common.c base64.c crc32.c reqid.c
OBJS = $(CFILES:%.c=%.o)
HDRS = dynstr.h plat.h proto.h common.h base64.h crc32.h reqid.h
CFLAGS = -I$(PWD) -Wall -Wextra -Werror -g -O2
LDLIBS =

BINDIR = /usr/sbin
MANSECT = 1m
MANDIR = /usr/share/man/man$(MANSECT)
DESTDIR = $(PWD)/proto

PROGS = \
	mdata-get \
	mdata-list \
	mdata-put \
	mdata-delete

PROTO_PROGS = \
	$(PROGS:%=$(DESTDIR)$(BINDIR)/%)

PROTO_MANPAGES = \
	$(PROGS:%=$(DESTDIR)$(MANDIR)/%.$(MANSECT))

INSTALL_TARGETS = \
	$(PROTO_PROGS) \
	$(PROTO_MANPAGES)

#
# Platform-specific definitions
#

ifeq ($(UNAME_S),SunOS)
CFLAGS += -D__HAVE_BOOLEAN_T
CFILES += plat/sunos.c plat/unix_common.c
HDRS += plat/unix_common.h
LDLIBS += -lnsl -lsocket -lsmbios
PLATFORM_OK = true
GNUTAR = gtar
endif

ifeq ($(UNAME_S),Linux)
CFILES += plat/linux.c plat/unix_common.c
HDRS += plat/unix_common.h
PLATFORM_OK = true
MANSECT = 1
INSTALL_TARGETS += $(DESTDIR)/lib/smartdc/mdata-get
PKGNAME = joyent-mdata-client
endif

ifeq ($(UNAME_S),FreeBSD)
CC = cc

CTFMERGE = /usr/bin/true
CTFCONVERT = /usr/bin/true

CFLAGS += -Wno-typedef-redefinition
CFILES += plat/bsd.c plat/unix_common.c
HDRS += plat/unix_common.h
PLATFORM_OK = true
MANSECT = 1
endif

ifeq ($(UNAME_S),NetBSD)
CC != if [ -x /usr/bin/clang ]; then echo /usr/bin/clang; else echo /usr/bin/gcc; fi

CTFMERGE != if [ -x /usr/bin/ctfmerge ]; then echo /usr/bin/ctfmerge; else echo /usr/bin/true; fi
CTFCONVERT != if [ -x /usr/bin/ctfconvert ]; then echo /usr/bin/ctfconvert; else echo /usr/bin/true; fi

CFILES += plat/bsd.c plat/unix_common.c
HDRS += plat/unix_common.h
PLATFORM_OK = true
MANSECT = 1
endif

ifeq ($(UNAME_S),OpenBSD)
CC = cc

CTFMERGE = /usr/bin/true
CTFCONVERT = /usr/bin/true

CFILES += plat/bsd.c plat/unix_common.c
HDRS += plat/unix_common.h
PLATFORM_OK = true
MANSECT = 1
endif

ifeq ($(UNAME_S),CYGWIN_NT-6.3)
CFILES += plat/cygwin.c plat/unix_common.c
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
install:	$(INSTALL_TARGETS)

$(DESTDIR)$(BINDIR)/%: %
	@mkdir -p $(DESTDIR)$(BINDIR)
	cp $< $@
	touch $@

$(DESTDIR)$(MANDIR)/%.$(MANSECT): man/man1m/%.1m
	@mkdir -p $(DESTDIR)$(MANDIR)
	sed 's/__SECT__/$(MANSECT)/g' < $< > $@

$(DESTDIR)/lib/smartdc/mdata-%:
	@mkdir -p $$(dirname $@)
	@rm -f $@
	ln -s $(BINDIR)/$$(basename $@) $@

#
# SmartOS (smartos-live) Package Manifest Targets
#

.PHONY: manifest
manifest:
	cp manifest $(DESTDIR)/$(DESTNAME)

.PHONY: mancheck_conf
mancheck_conf:

.PHONY: update
update:
	git pull --rebase

#
# Debian Package Targets
#
#

.PHONY: package-debian
package-debian:
	debuild -us -uc

source-tarball:
	if [ -z $(RELEASE_DIRECTORY) ]; then \
		echo "error: define RELEASE_DIRECTORY" >&2; \
		exit 1; \
	fi
	if [ -z $(PKGNAME) ]; then \
		echo "error: define PKGNAME" >&2; \
		exit 1; \
	fi
	$(GNUTAR) \
		-zc -f $(RELEASE_DIRECTORY)/$(PKGNAME)_$(VERSION).orig.tar.gz \
		--exclude=.git \
		--transform 's,^,$(PKGNAME)_$(VERSION)/,' \
		*

#
# Cleanup Targets
#

.PHONY:	clean
clean:
	rm -f $(PROGS) $(OBJS)

.PHONY:	clobber
clobber:	clean
	rm -rf $(PWD)/proto
