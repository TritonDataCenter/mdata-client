/*
 * Copyright (c) 2013, Joyent, Inc. All rights reserved.
 */

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <err.h>
#include <errno.h>
#include <string.h>
#include <strings.h>

#include "common.h"
#include "dynstr.h"
#include "plat.h"
#include "proto.h"

typedef enum mdata_exit_codes {
	MDEC_SUCCESS = 0,
	MDEC_NOTFOUND = 1,
	MDEC_ERROR = 2,
	MDEC_USAGE_ERROR = 3,
	MDEC_TRY_AGAIN = 10
} mdata_exit_codes_t;

static int
print_response(mdata_response_t mdr, string_t *data)
{
	switch (mdr) {
	case MDR_SUCCESS:
		fprintf(stdout, "%s\n", dynstr_cstr(data));
		return (MDEC_SUCCESS);
	case MDR_NOTFOUND:
		fprintf(stderr, "No metadata\n");
		return (MDEC_NOTFOUND);
	case MDR_UNKNOWN:
		fprintf(stderr, "Error getting metadata: %s\n",
		    dynstr_cstr(data));
		return (MDEC_ERROR);
	default:
		ABORT("print_response: UNKNOWN RESPONSE\n");
		return (MDEC_ERROR);
	}
}

int
main(int argc __UNUSED, char **argv __UNUSED)
{
	mdata_proto_t *mdp;
	mdata_response_t mdr;
	string_t *data;
	char *errmsg = NULL;

	if (proto_init(&mdp, &errmsg) != 0) {
		fprintf(stderr, "ERROR: could not initialise protocol: %s\n",
		    errmsg);
		return (MDEC_ERROR);
	}

	if (proto_execute(mdp, "KEYS", NULL, &mdr, &data) != 0) {
		fprintf(stderr, "ERROR: could not execute KEYS\n");
		return (MDEC_ERROR);
	}

	return (print_response(mdr, data));
}
