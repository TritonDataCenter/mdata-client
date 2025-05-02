/*
 * Copyright (c) 2019, Joyent, Inc.
 * See LICENSE file for copyright and license details.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <err.h>

#include <poll.h>

#include "common.h"
#include "plat.h"
#include "dynstr.h"
#include "plat/unix_common.h"

#define	SERIAL_DEVICE	"/dev/ttyS1"
#define	MAX_READ_SZ	16

struct mdata_plat {
	struct pollfd mpl_poll;
	int mpl_conn;
};


int
plat_send(mdata_plat_t *mpl, string_t *data)
{
	size_t len = dynstr_len(data);

	if (write(mpl->mpl_conn, dynstr_cstr(data), len) != (ssize_t)len)
		return (-1);

	return (0);
}

int
plat_recv(mdata_plat_t *mpl, string_t *data, int timeout_ms)
{
	for (;;) {
		struct pollfd *mpl_poll = &mpl->mpl_poll;
		short revents = 0;

		if (poll(mpl_poll, 1, timeout_ms) == -1) {
			fprintf(stderr, "poll error: %d\n", errno);
			if (errno == EINTR)
				return (-1);
			err(1, "POLL_WAIT ERROR");
		}

		revents = mpl_poll->revents;

		if (mpl_poll->fd < 0 || revents == 0) {
			fprintf(stderr, "plat_recv timeout\n");
			return (-1);
		}

		if (revents & POLLIN) {
			char buf[MAX_READ_SZ + 1];
			static char buf_prev[MAX_READ_SZ + 1] = "";
			ssize_t sz;

			if ((sz = read(mpl_poll->fd, buf, MAX_READ_SZ)) > 0) {
				boolean_t terminate = B_FALSE;
				char* curr = buf;
				char* rem = buf;
				char* end = buf + sz;

				if (*buf_prev != '\0') {
					dynstr_append(data, buf_prev);
					*buf_prev = '\0';
				}

				while ((rem = memchr(curr, '\n', end - curr))) {
					*rem = '\0';
					dynstr_append(data, curr);
					curr = rem;
					terminate = B_TRUE;
				}

				if (terminate == B_TRUE) {
					return (0);
				}

				*end = '\0';
				strcpy(buf_prev, curr);
			}
		}

		if (revents & POLLERR) {
			fprintf(stderr, "POLLERR\n");
			return (-1);
		}

		if (revents & POLLHUP) {
			fprintf(stderr, "POLLHUP\n");
			return (-1);
		}
	}

	return (-1);
}

void
plat_fini(mdata_plat_t *mpl)
{
	if (mpl != NULL) {
		if (mpl->mpl_conn != -1)
			(void) close(mpl->mpl_conn);
		free(mpl);
	}
}

static int
plat_send_reset(mdata_plat_t *mpl)
{
	int ret = -1;
	string_t *str = dynstr_new();

	dynstr_append(str, "\n");
	if (plat_send(mpl, str) != 0)
		goto bail;
	dynstr_reset(str);

	if (plat_recv(mpl, str, 2000) != 0)
		goto bail;

	if (strcmp(dynstr_cstr(str), "invalid command") != 0)
		goto bail;

	ret = 0;

bail:
	dynstr_free(str);
	return (ret);
}

int
plat_is_interactive(void)
{
	return (unix_is_interactive());
}

int
plat_init(mdata_plat_t **mplout, char **errmsg, int *permfail)
{
	mdata_plat_t *mpl = NULL;

	if ((mpl = calloc(1, sizeof (*mpl))) == NULL) {
		*errmsg = "Could not allocate memory.";
		*permfail = 1;
		goto bail;
	}

	if (unix_open_serial(SERIAL_DEVICE, &mpl->mpl_poll.fd, errmsg,
	    permfail) != 0) {
		goto bail;
	}

	mpl->mpl_conn = mpl->mpl_poll.fd;
	mpl->mpl_poll.events = POLLIN | POLLERR | POLLHUP;

	if (plat_send_reset(mpl) == -1) {
		*errmsg = "Could not do active reset.";
		goto bail;
	}

	*mplout = mpl;

	return (0);

bail:
	plat_fini(mpl);
	return (-1);
}
