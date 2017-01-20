/*
 * Copyright (c) 2013, Joyent, Inc.
 * See LICENSE file for copyright and license details.
 */

#ifndef _UNIX_COMMON_H
#define	_UNIX_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

#include "plat.h"
#include "dynstr.h"

int unix_open_serial(const char *devpath, int *outfd, const char **errmsg, int *permfail);
int unix_send_reset(mdata_plat_t *mpl);
int unix_is_interactive(void);


#ifdef __cplusplus
}
#endif

#endif /* _UNIX_COMMON_H */
