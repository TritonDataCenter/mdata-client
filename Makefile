
CC = gcc

UNAME_S := $(shell uname -s)
PLATFORM_OK = false

CFILES = dynstr.c proto.c common.c
HDRS = dynstr.h plat.h proto.h common.h
CFLAGS = -I$(PWD) -Wall -Wextra -g -O0
LDLIBS =

ifeq ($(UNAME_S),SunOS)
CFILES += plat/sunos.c plat/unix_common.c
LDLIBS += -lnsl -lsocket -lsmbios
PLATFORM_OK = true
endif

ifeq ($(UNAME_S),Linux)
CFILES += plat/linux.c plat/unix_common.c
PLATFORM_OK = true
endif

ifeq ($(PLATFORM_OK),false)
$(error Unknown platform: $(UNAME_S))
endif


.PHONY:	all
all:	mdata-get mdata-list

mdata-get:	$(CFILES) $(HDRS) mdata_get.c
	$(CC) $(CFLAGS) $(LDLIBS) -o $@ mdata_get.c $(CFILES)

mdata-list:	$(CFILES) $(HDRS) mdata_list.c
	$(CC) $(CFLAGS) $(LDLIBS) -o $@ mdata_list.c $(CFILES)

.PHONY:	clean
clean:
	rm -f mdata-get mdata-list

