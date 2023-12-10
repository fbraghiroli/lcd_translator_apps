/*
lcd_translator_apps

Copyright (C) 2023 Federico Braghiroli

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <sys/ioctl.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#include <nuttx/lcd/slcd_ioctl.h>
#include <nuttx/lcd/slcd_codec.h>

#include "utils.h"
#include "proto.h"

#define SLCD_BUFSIZE 256

/* NuttX interface */

struct ctrl_slcd {
	/* BE CAREFUL: stream must be the first element of the struct.
	 * This allows to force a cast to this struct on streams callbacks
	 * functions (putc, flush). */
	struct lib_outstream_s stream;

	int fd;
	struct slcd_attributes_s attr;
	uint8_t buffer[SLCD_BUFSIZE+1];
};

static void slcd_dumpbuffer(const uint8_t *buffer, unsigned int buflen)
{
	/* From NuttX slcd example */
	int i, j, k;

	for (i = 0; i < buflen; i += 32) {
		info("%04x: ", i);
		for (j = 0; j < 32; j++) {
			k = i + j;

			if (j == 16)
				info(" ");

			if (k < buflen)
				info("%02x", buffer[k]);
			else
				info("  ");
		}

		info(" ");
		for (j = 0; j < 32; j++) {
			k = i + j;

			if (j == 16)
				info(" ");

			if (k < buflen) {
				if (buffer[k] >= 0x20 && buffer[k] < 0x7f)
					info("%c", buffer[k]);
				else
					info(".");
			}
		}
		info("\n");
	}
}

static int cbk_slcd_flush(struct lib_outstream_s *stream)
{
	struct ctrl_slcd *priv = (struct ctrl_slcd *)stream;
	const uint8_t *buffer;
	ssize_t nwritten;
	ssize_t remaining;

	/* From NuttX slcd example, not quite sure what to return when
	 * it fails.
	 */

	remaining = stream->nput;
	buffer = priv->buffer;

	//info("slcd buffer dump\n");
	//slcd_dumpbuffer(buffer, remaining);

	while (remaining > 0) {
		nwritten = write(priv->fd, buffer, remaining);
		if (nwritten < 0) {
			if (errno != EINTR)
				error("write failed: %d\n", -errno);
			/* continue anyway... don't know how to signal error to NuttX lib */
		}
		else {
			remaining -= nwritten;
			buffer    += nwritten;
		}
	}

	/* Reset the stream */
	stream->nput = 0;

	return OK;
}

static void cbk_slcd_putc(struct lib_outstream_s *stream, int ch)
{
	struct ctrl_slcd *priv = (struct ctrl_slcd *)stream;

	/* Write the character to the buffer */

	priv->buffer[stream->nput] = (uint8_t)ch;
	stream->nput++;
	priv->buffer[stream->nput] = '\0';

	/* If the buffer is full, flush it */

	if (stream->nput >= SLCD_BUFSIZE)
		cbk_slcd_flush(stream);
}

static void cbk_slcd_puts(struct lib_outstream_s *outstream, const char *str)
{
	for (; *str; str++)
		slcd_put((int)*str, outstream);
}

/* rows and cols are zero based */
static void slcd_set_curpos(struct ctrl_slcd *hndl, uint8_t r, uint8_t c)
{
	struct ctrl_slcd *priv = hndl;
	slcd_encode(SLCDCODE_HOME, 0, &priv->stream);
	/* HACK:
	 * Hardware cursor seems to jump by one line on 20x2 display controllers,
	 * so to be sure to go up to the first row, just double the jump */
	slcd_encode(SLCDCODE_UP, priv->attr.nrows*2, &priv->stream);
	cbk_slcd_flush(&priv->stream);
	slcd_encode(SLCDCODE_RIGHT, c, &priv->stream);
	slcd_encode(SLCDCODE_DOWN, r, &priv->stream);
}

struct ctrl_slcd* ctrl_slcd_init(const char *dev)
{
	int ret = 0;
	struct ctrl_slcd *priv;

	priv = calloc(1, sizeof(struct ctrl_slcd));
	if (priv == NULL)
		return NULL;

	priv->fd = open(dev, O_RDWR);
	if (priv->fd < 0) {
		ret = -errno;
		goto exit_alloc;
	}

	ret = ioctl(priv->fd, SLCDIOC_GETATTRIBUTES, (unsigned long)&priv->attr);
	if (ret < 0) {
		error("failed to get slcd attributes\n");
		ret = -errno;
		goto exit_alloc;
	}

	info("slcd attributes:\n");
	info("\trows: %d columns: %d nbars: %d\n",
		priv->attr.nrows, priv->attr.ncolumns, priv->attr.nbars);
	info("\tmax contrast: %d max brightness: %d\n",
		priv->attr.maxcontrast, priv->attr.maxbrightness);

	priv->stream.putc = cbk_slcd_putc;
	priv->stream.flush = cbk_slcd_flush;

	slcd_encode(SLCDCODE_CLEAR, 0, &priv->stream);
	cbk_slcd_flush(&priv->stream);

	return priv;

exit_alloc:
	free(priv);
	error("init err: %d\n", ret);
	return NULL;
}

int ctrl_slcd_deinit(struct ctrl_slcd *hndl)
{
	if (!hndl)
		return -EINVAL;
	close(hndl->fd);
	free(hndl);
	return 0;
}

int ctrl_slcd_cmd(struct ctrl_slcd *hndl, const struct proto_cmd_data *cmd)
{
	struct ctrl_slcd *priv = hndl;
	struct slcd_curpos_s attr_pos;

	switch(cmd->cmd) {
	case PROTO_CMD_ASCII:
		/* We need to save the position *before* writing because on 20x4
		 * display, after writing to the last column the row value returned
		 * by the controller is not the next line but the is the current +2.
		 * To simplify the line wrap handling with different displays and
		 * controllers we manually implement line wrap. */
		ioctl(hndl->fd, SLCDIOC_CURPOS, (unsigned long)&attr_pos);

		info("ascii: 0x%02x %c\n", cmd->data.ascii, cmd->data.ascii);
		slcd_put(cmd->data.ascii, &priv->stream);
		cbk_slcd_flush(&priv->stream);

		/* Check if we wrote on the last column */
		if (attr_pos.column == hndl->attr.ncolumns-1) {
			if (attr_pos.row < hndl->attr.nrows-1)
				attr_pos.row++;
			else
				attr_pos.row = 0;
			/* This might be slow, evaluate to use SLCDCODE_DOWN when not
			 * wrapping on the last line. */
			slcd_set_curpos(priv, attr_pos.row, 0);
		}
		break;
	case PROTO_CMD_GET_SN:
	case PROTO_CMD_GET_FW_VER:
	case PROTO_CMD_GET_DISPLAY_TYPE:
		/* No way those can get implemented */
		break;
	case PROTO_CMD_AUTO_LINE_WRAP_ON:
		/* software implementation */
		info("TODO: auto line wrap on\n");
		break;
	case PROTO_CMD_AUTO_LINE_WRAP_OFF:
		/* software implementation */
		info("TODO: auto line wrap off\n");
		break;
	case PROTO_CMD_AUTO_SCROLL_ON:
		/* software implementation */
		info("TODO: auto scroll on\n");
		break;
	case PROTO_CMD_AUTO_SCROLL_OFF:
		/* software implementation */
		info("TODO: auto scroll off\n");
		break;
	case PROTO_CMD_SET_CURSOR_POS:
		dbg("Cursor: %d %d\n", cmd->data.pos.row-1, cmd->data.pos.col-1);
		slcd_set_curpos(priv, cmd->data.pos.row-1, cmd->data.pos.col-1);
		break;
	case PROTO_CMD_SEND_CURSOR_HOME:
		info("Cursor home\n");
		/* Go to the top left */
		/* SLCDCODE_HOME only moves the cursor at home for the current row */
		slcd_encode(SLCDCODE_HOME, 0, &priv->stream);
		/* so, we need also to go up */
		slcd_encode(SLCDCODE_UP, priv->attr.nrows, &priv->stream);
		break;
	case PROTO_CMD_UNDERLINE_CURSOR_ON:
	case PROTO_CMD_UNDERLINE_CURSOR_OFF:
		/* TODO: check if supported by the display */
		break;
	case PROTO_CMD_BLINK_CURSOR_ON:
		//slcd_encode(SLCDCODE_BLINKSTART, 0, &priv->stream);
		break;
	case PROTO_CMD_BLINK_CURSOR_OFF:
		//slcd_encode(SLCDCODE_BLINKOFF, 0, &priv->stream);
		break;
	case PROTO_CMD_CURSOR_LEFT:
		slcd_encode(SLCDCODE_LEFT, 1, &priv->stream);
		break;
	case PROTO_CMD_CURSOR_RIGHT:
		slcd_encode(SLCDCODE_RIGHT, 1, &priv->stream);
		break;
	case PROTO_CMD_ADD_CUSTOM_CHAR:
		info("TODO: add custom char\n");
		/* TODO */
		break;
	case PROTO_CMD_CLR_DISPLAY:
		slcd_encode(SLCDCODE_CLEAR, 0, &priv->stream);
		break;
	case PROTO_CMD_SET_CONTRAST: /* data */
		info("contrast not supported\n");
		break;
	case PROTO_CMD_BACKLIGHT_ON: /* data */
		/* Ignore 'on' time */
		//ioctl(priv->fd, SLCDIOC_SETBRIGHTNESS, priv->attr.maxbrightness);
		break;
	case PROTO_CMD_BACKLIGHT_OFF:
		//ioctl(priv->fd, SLCDIOC_SETBRIGHTNESS, 0);
		break;
	case PROTO_CMD_BACKLIGHT_LVL: /* data */
		/* Not supported by the hardware */
		//ioctl(priv->fd, SLCDIOC_SETBRIGHTNESS, priv->attr.maxbrightness);
		break;
	default:
		info("cmd %d not implemented\n", cmd->cmd);
		break;
	}

	if (cmd->cmd != PROTO_CMD_ASCII) {
		/* All cmds but ascii char shall be executed immediately. */
		cbk_slcd_flush(&priv->stream);
	}

	return 0;
}
