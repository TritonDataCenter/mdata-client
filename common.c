/*
 * See LICENSE file for copyright and license details.
 *
 * Copyright (c) 2013 Joyent, Inc.
 * Copyright (c) 2024 MNX Cloud, Inc.
 */

#include <stdio.h>
#include <stdlib.h>

#include "common.h"

int
print_and_abort(const char *message, const char *file, int line)
{
	(void) fprintf(stderr, "ASSERT: %s, file: %s @ line %d\n",
	    message, file, line);

	abort();

	return (0);
}
