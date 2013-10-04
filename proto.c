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

#define	RECV_TIMEOUT_MS	2000

typedef enum mdata_proto_state {
	MDPS_MESSAGE_HEADER = 1,
	MDPS_MESSAGE_DATA,
	MDPS_READY,
	MDPS_ERROR
} mdata_proto_state_t;

typedef struct mdata_command {
	string_t *mdc_request;
	string_t *mdc_response_data;
	mdata_response_t mdc_response;
	int mdc_done;
} mdata_command_t;

typedef struct mdata_proto {
	mdata_plat_t *mdp_plat;
	mdata_command_t *mdp_command;
	mdata_proto_state_t mdp_state;
} mdata_proto_t;

static int proto_send(mdata_proto_t *mdp);
static int proto_recv(mdata_proto_t *mdp);

static int
proto_reset(mdata_proto_t *mdp)
{
	char *errmsg = NULL;
	int permfail = 0;

	if (mdp->mdp_plat != NULL) {
		plat_fini(mdp->mdp_plat);
		mdp->mdp_plat = NULL;
	}

retry:
	if (plat_init(&mdp->mdp_plat, &errmsg, &permfail) == -1) {
		if (permfail) {
			fprintf(stderr, "ERROR: while resetting connection: "
			    "%s\n", errmsg);
			return (-1);
		} else {
			sleep(1);
			goto retry;
		}
	}

	return (0);
}

static void
process_input(mdata_proto_t *mdp, string_t *input)
{
	const char *cstr = dynstr_cstr(input);

	switch (mdp->mdp_state) {
	case MDPS_MESSAGE_HEADER:
		if (strcmp(cstr, "NOTFOUND") == 0) {
			mdp->mdp_state = MDPS_READY;
			mdp->mdp_command->mdc_response = MDR_NOTFOUND;
			mdp->mdp_command->mdc_done = 1;

		} else if (strcmp(cstr, "SUCCESS") == 0) {
			mdp->mdp_state = MDPS_MESSAGE_DATA;
			mdp->mdp_command->mdc_response = MDR_SUCCESS;

		} else if (strcmp(cstr, "invalid command") == 0) {
			mdp->mdp_state = MDPS_READY;
			mdp->mdp_command->mdc_response = MDR_INVALID_COMMAND;
			mdp->mdp_command->mdc_done = 1;

		} else {
			mdp->mdp_state = MDPS_READY;
			dynstr_append(mdp->mdp_command->mdc_response_data, cstr);
			mdp->mdp_command->mdc_response = MDR_UNKNOWN;
			mdp->mdp_command->mdc_done = 1;

		}
		break;

	case MDPS_MESSAGE_DATA:
		if (strcmp(cstr, ".") == 0) {
			mdp->mdp_state = MDPS_READY;
			mdp->mdp_command->mdc_done = 1;
		} else {
			string_t *respdata = mdp->mdp_command->mdc_response_data;
			int offs = cstr[0] == '.' ? 1 : 0;
			if (dynstr_len(respdata) > 0)
				dynstr_append(respdata, "\n");
			dynstr_append(respdata, cstr + offs);
		}
		break;

	case MDPS_READY:
	case MDPS_ERROR:
		break;

	default:
		ABORT("process_input: UNKNOWN STATE\n");
	}
}

static int
proto_send(mdata_proto_t *mdp)
{
	VERIFY(mdp->mdp_command);

	if (plat_send(mdp->mdp_plat, mdp->mdp_command->mdc_request) == -1) {
		mdp->mdp_state = MDPS_ERROR;
		return (-1);
	}

	/*
	 * Wait for response header from remote peer:
	 */
	mdp->mdp_state = MDPS_MESSAGE_HEADER;

	return (0);
}

static int
proto_recv(mdata_proto_t *mdp)
{
	int ret = -1;
	string_t *line = dynstr_new();

	for (;;) {
		if (plat_recv(mdp->mdp_plat, line, RECV_TIMEOUT_MS) == -1) {
			mdp->mdp_state = MDPS_ERROR;
			goto bail;
		}

		process_input(mdp, line);
		dynstr_reset(line);

		if (mdp->mdp_command->mdc_done)
			break;
	}

	ret = 0;

bail:
	dynstr_free(line);
	return (ret);
}

int
proto_execute(mdata_proto_t *mdp, const char *command, const char *argument,
    mdata_response_t *response, string_t **response_data)
{
	mdata_command_t mdc;

	/*
	 * Initialise new command structure:
	 */
	bzero(&mdc, sizeof (mdc));
	mdc.mdc_request = dynstr_new();
	mdc.mdc_response_data = dynstr_new();
	mdc.mdc_response = MDR_PENDING;
	mdc.mdc_done = 0;

	VERIFY0(mdp->mdp_command);
	mdp->mdp_command = &mdc;

	/*
	 * Generate request to send to remote peer:
	 */
	dynstr_append(mdc.mdc_request, command);
	if (argument != NULL) {
		dynstr_append(mdc.mdc_request, " ");
		dynstr_append(mdc.mdc_request, argument);
	}
	dynstr_append(mdc.mdc_request, "\n");

	for (;;) {
		/*
		 * Attempt to send the request to the remote peer:
		 */
		if (mdp->mdp_state == MDPS_ERROR || proto_send(mdp) != 0 ||
		    proto_recv(mdp) != 0) {
			/*
			 * Discard existing response data and reset the command
			 * state:
			 */
			dynstr_reset(mdp->mdp_command->mdc_response_data);
			mdc.mdc_response = MDR_PENDING;

			/*
			 * We could not send the request, so reset the stream
			 * and try again:
			 */
			fprintf(stderr, "receive timeout, resetting "
			    "protocol...\n");
			if (proto_reset(mdp) == -1) {
				/*
				 * We could not do a reset, so abort the whole
				 * thing.
				 */
				goto bail;
			} else {
				/*
				 * We were able to reset OK, so keep trying.
				 */
				continue;
			}
		}

		if (mdp->mdp_state != MDPS_READY)
			ABORT("proto state not MDPS_READY\n");

		/*
		 * We were able to send a command and receive a response.
		 * Examine the response and decide what to do:
		 */
		*response = mdc.mdc_response;
		*response_data = mdc.mdc_response_data;
		dynstr_free(mdc.mdc_request);
		return (0);
	}

bail:
	dynstr_free(mdc.mdc_request);
	dynstr_free(mdc.mdc_response_data);
	return (-1);
}

int
proto_init(mdata_proto_t **out, char **errmsg)
{
	int permfail = 0;
	mdata_proto_t *mdp;

	if ((mdp = calloc(1, sizeof (*mdp))) == NULL)
		return (-1);

retry:
	if (plat_init(&mdp->mdp_plat, errmsg, &permfail) == -1) {
		if (permfail) {
			free(mdp);
			return (-1);
		} else {
			sleep(1);
			fprintf(stderr, "retry platform-level init\n");
			goto retry;
		}
	}

	*out = mdp;

	return (0);
}
