/*
 * Copyright (c) 2013, Joyent, Inc. All rights reserved.
 */

#ifndef _BASE64_H
#define	_BASE64_H

#include "dynstr.h"

#ifdef __cplusplus
extern "C" {
#endif

void base64_encode(const char *, size_t, string_t *);
int base64_decode(const char *, size_t, string_t *);

#ifdef __cplusplus
}
#endif

#endif /* _BASE64_H */
