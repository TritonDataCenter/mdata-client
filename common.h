/*
 * Copyright (c) 2013, Joyent, Inc. All rights reserved.
 */

#ifndef _COMMON_H
#define	_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

#define	__UNUSED	__attribute__((unused))

#define	VERIFY(EX)	((void)((EX) || print_and_abort("EX", __FILE__,__LINE__)))
#define	VERIFY0(EX)	((void)((EX) && print_and_abort("EX", __FILE__, __LINE__)))

int print_and_abort(const char *, const char *, int);

#ifdef __cplusplus
}
#endif

#endif /* _COMMON_H */
