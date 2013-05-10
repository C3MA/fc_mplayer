/* ------------------------------------------------------------------------- */

/* SOURCE:
 * md5sum video output driver
 *
 * Copyright (C) 2004, 2005, 2006 Ivo van Poorten
 *
 * This file is part of MPlayer.
 *
 * MPlayer is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * MPlayer is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with MPlayer; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.


MODIFICATIONS:
also added in libvo/video_out.c

extern const vo_functions_t video_out_fc1;
(also add this variable to the list)


at the Makefile add the following line (next to the other modules):
SRCS_MPLAYER-$(FC1)           += libvo/vo_fc1.c

And define:
FC1 = yes 
in config.mk
 */

/* ------------------------------------------------------------------------- */

/* Global Includes */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/* ------------------------------------------------------------------------- */

/* Local Includes */

#include "config.h"
#include "subopt-helper.h"
#include "mp_msg.h"
#include "video_out.h"
#include "video_out_internal.h"
#include "mp_core.h"			/* for exit_player() */
#include "help_mp.h"

#include "fc1/fcclient.h"
/* ------------------------------------------------------------------------- */

/* Info */

static const vo_info_t info=
{
	"Output to Fullcircle (http://www.ccc-mannheim.de/wiki/Fullcircle)",
	"fc1",
	"C3MA",
	""
};

const LIBVO_EXTERN (fc1)

/* ------------------------------------------------------------------------- */

/* Global Variables */

char *fc_dumpfile = NULL;
char *fc_ip = NULL;

FILE *dumpfile_fd;

fcclient_t * client;
uint8_t* frame;

/* ------------------------------------------------------------------------- */

/** \brief Pre-initialisation.
 *
 * This function is called before initialising the video output driver. It
 * parses all suboptions and sets variables accordingly. If an error occurs
 * (like an option being out of range, not having any value or an unknown
 * option is stumbled upon) the player will exit. It also sets default
 * values if necessary.
 *
 * \param arg   A string containing all the suboptions passed to the video
 *              output driver.
 *
 * \return 0    All went well.
 */

static int preinit(const char *arg)
{
    const opt_t subopts[] = {
        {"outfile",     OPT_ARG_MSTRZ,    &fc_dumpfile,   NULL},
	{"ip",     OPT_ARG_MSTRZ,    &fc_ip,   NULL},
        {NULL, 0, NULL, NULL}
    };

    mp_msg(MSGT_VO, MSGL_V, "%s: %s\n", info.short_name,
           "Parsing suboptions.");

    fc_dumpfile = strdup("fullcicle_dump1");
    if (subopt_parse(arg, subopts) != 0) {
        return -1;
    }

    mp_msg(MSGT_VO, MSGL_V, "%s: outfile --> %s\n", info.short_name,
                                                            fc_dumpfile);

    mp_msg(MSGT_VO, MSGL_V, "%s: %s\n", info.short_name,
           "Suboptions parsed OK.");
    return 0;
}

/* ------------------------------------------------------------------------- */

/** \brief Configure the video output driver.
 *
 * This functions configures the video output driver. It opens the output
 * file to which this driver will write all the MD5 sums. If something
 * goes wrong, the player will exit.
 *
 *  \return 0             All went well.
 */

static int config(uint32_t width, uint32_t height, uint32_t d_width,
                       uint32_t d_height, uint32_t flags, char *title,
                       uint32_t format)
{
    int success, success2;

    if (vo_config_count > 0 ) { /* Already configured */
        return 0;
    }

    if (strcmp(fc_dumpfile, "-") == 0)
        dumpfile_fd = stdout;
    else
    if ( (dumpfile_fd = fopen(fc_dumpfile, "w") ) == NULL ) {
        mp_msg(MSGT_VO, MSGL_ERR, "\n%s: %s\n", info.short_name,
                MSGTR_VO_CantCreateFile);
        mp_msg(MSGT_VO, MSGL_ERR, "%s: %s: %s\n",
                info.short_name, MSGTR_VO_GenericError, strerror(errno) );
        exit_player(EXIT_ERROR);
    }

    /* Add fullcirce stuff */
    client = fcclient_new();
    fprintf(dumpfile_fd, "# IP %s\n", fc_ip);
    success = fcclient_open(client, fc_ip);
    if (!success) {
	fprintf(dumpfile_fd, "Cannot connect to wall\n");
	return 1;
    }
    else
    {
        do {
                /* call this function until we were successfull in receiving something */
                success = fcclient_processNetwork(client);
                fprintf(dumpfile_fd, "Network: %d\n", success);
                /*FIXME update the function using "select()" with an timeout */
        } while (!success);
	/* the server has answered */

        success2 = fcclient_start(client);
        fprintf(dumpfile_fd, "Start: %d\n", success2);
	frame = malloc(client->width * client->height * 20);

	printf("======== Fullcircle - Waiting for start signal ==========\n", success);
	do
	{
		success = fcclient_processNetwork(client);
		fprintf(dumpfile_fd, "Received data [%d]\n", success);
		printf(".");
	}
	while (!client->connected);
	printf("\n");

    }
    return 0;
}

/* ------------------------------------------------------------------------- */

static int draw_frame(uint8_t *src[])
{
    mp_msg(MSGT_VO, MSGL_V, "%s: draw_frame() is called!\n", info.short_name);
    return -1;
}

/* ------------------------------------------------------------------------- */

static uint32_t draw_image(mp_image_t *mpi)
{
    unsigned char md5sum[16];
    uint32_t width = mpi->w;
    uint32_t height = mpi->h;
    uint8_t *rgbimage = mpi->planes[0];
    uint8_t *planeY = mpi->planes[0];
    uint8_t *planeU = mpi->planes[1];
    uint8_t *planeV = mpi->planes[2];
    uint32_t strideY = mpi->stride[0];
    uint32_t strideU = mpi->stride[1];
    uint32_t strideV = mpi->stride[2];

    unsigned int i;

    /* FC variables */
    int success, x1, y1, rgbPos;

    if (mpi->flags & MP_IMGFLAG_PLANAR) { /* Planar */
            return VO_FALSE;
    } else { /* Packed */
        if (mpi->flags & MP_IMGFLAG_YUV) { /* Packed YUV */
            return VO_FALSE;
        } else { /* Packed RGB */
#if 0
		/*FIXME here RGB of each pixel must be extracted  */
		for(i=0; i < 3; i++)
		{
		    fprintf(dumpfile_fd, "%x %x %x \t", rgbimage[(i * 3) + 0], rgbimage[(i * 3) + 1], rgbimage[(i * 3) + 2] );
		}
		fprintf(dumpfile_fd, "\n");
#endif
		if (client->connected) {
                        for (x1=0; x1 < client->width; x1++) {
                                for (y1=0; y1 < client->height; y1++) {
					rgbPos = (y1 * width) + x1;
					fcclient_addPixel(client, frame, 
						rgbimage[(rgbPos  * 3) + 0], 
						rgbimage[(rgbPos  * 3) + 1],
						rgbimage[(rgbPos  * 3) + 2], x1, y1);
                                }
                        }

			/* Now we need to send some nice frames to the wall */
                        fcclient_sendFrame(client, frame);
		}
	    fprintf(dumpfile_fd, "Send data\n");
            return VO_TRUE;
        }
    }

    return VO_FALSE;
}

/* ------------------------------------------------------------------------- */

int draw_slice(uint8_t *src[], int stride[], int w, int h,
                           int x, int y)
{
    return 0;
}

/* ------------------------------------------------------------------------- */

int query_format(uint32_t format)
{
    switch (format) {
        case IMGFMT_RGB24:
//        case IMGFMT_YV12:
            return VFCAP_CSP_SUPPORTED|VFCAP_CSP_SUPPORTED_BY_HW;
        default:
            return 0;
    }
}

/* ------------------------------------------------------------------------- */

int control(uint32_t request, void *data)
{
    switch (request) {
        case VOCTRL_QUERY_FORMAT:
            return query_format(*((uint32_t*)data));
        case VOCTRL_DRAW_IMAGE:
            return draw_image(data);
    }
    return VO_NOTIMPL;
}

/* ------------------------------------------------------------------------- */

void uninit(void)
{
    free(fc_dumpfile);
    fc_dumpfile = NULL;
    if (dumpfile_fd && dumpfile_fd != stdout) fclose(dumpfile_fd);
}

/* ------------------------------------------------------------------------- */

void check_events(void)
{
}

/* ------------------------------------------------------------------------- */

void draw_osd(void)
{
}

/* ------------------------------------------------------------------------- */

void flip_page (void)
{
}
