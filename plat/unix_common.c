/*
 * See LICENSE file for copyright and license details.
 *
 * Copyright (c) 2013 Joyent, Inc.
 * Copyright (c) 2024 MNX Cloud, Inc.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <termios.h>
#include <unistd.h>

#include "common.h"
#include "dynstr.h"
#include "plat.h"
#include "unix_common.h"

int
unix_is_interactive(void)
{
	return (isatty(STDIN_FILENO) == 1);
}

static int
unix_raw_mode(int fd, const char **errmsg)
{
	struct termios tios;

	if (tcgetattr(fd, &tios) == -1) {
		*errmsg = "could not set raw mode on serial device";
		return (-1);
	}

	tios.c_iflag &= (tcflag_t)~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
	tios.c_oflag &= (tcflag_t)~(OPOST);
	tios.c_cflag |= (tcflag_t)(CS8);
	tios.c_cflag &= (tcflag_t)~(HUPCL);
	tios.c_lflag &= (tcflag_t)~(ECHO | ICANON | IEXTEN | ISIG);

	/*
	 * As described in "Case C: MIN = 0, TIME > 0" of termio(7I), this
	 * configuration will block waiting for at least one character, or
	 * the expiry of a 100 millisecond timeout:
	 */
	tios.c_cc[VMIN] = 0;
	tios.c_cc[VTIME] = 1;

	if (tcsetattr(fd, TCSAFLUSH, &tios) == -1) {
		*errmsg = "could not get attributes from serial device";
		return (-1);
	}

	return (0);
}

int
unix_open_serial(const char *devpath, int *outfd, const char **errmsg, int *permfail)
{
	int fd;
	char scrap[100];
	ssize_t sz;
	struct flock l;

	if ((fd = open(devpath, O_RDWR | O_EXCL |
	    O_NOCTTY)) == -1) {
		*errmsg = "Could not open serial device.";
		if (errno != EAGAIN && errno != EBUSY && errno != EINTR)
			*permfail = 1;
		return (-1);
	}

	/*
	 * Lock the serial port for exclusive access:
	 */
	l.l_type = F_WRLCK;
	l.l_whence = SEEK_SET;
	l.l_start = l.l_len = 0;
	if (fcntl(fd, F_SETLKW, &l) == -1) {
		*errmsg = "Could not lock serial device.";
		return (-1);
	}

	/*
	 * Set raw mode on the serial port:
	 */
	if (unix_raw_mode(fd, errmsg) == -1) {
		(void) close(fd);
		*permfail = 1;
		return (-1);
	}

	/*
	 * Because this is a shared serial line, we may be part way through
	 * a response from the remote peer.  Read (and discard) data until we
	 * cannot do so anymore:
	 */
	do {
		sz = read(fd, &scrap, sizeof (scrap));

		if (sz == -1 && errno != EAGAIN) {
			*errmsg = "Failed to flush serial port before use.";
			(void) close(fd);
			return (-1);
		}

	} while (sz > 0);

	*outfd = fd;

	return (0);
}
