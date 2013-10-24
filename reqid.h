/*
 * Copyright (c) 2013, Joyent, Inc. All rights reserved.
 */

#ifndef _REQID_H
#define	_REQID_H

#ifdef __cplusplus
extern "C" {
#endif

#define	REQID_LEN	9

int reqid_init(void);
void reqid_fini(void);
char * reqid(char *buf);

#ifdef __cplusplus
}
#endif

#endif /* _REQID_H */
