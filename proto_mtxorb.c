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

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "proto.h"

#define MTXORB_HEADER 0xfe

enum msg_fsm_states {
	MSG_FSM_NONE,
	MSG_FSM_HEADER,
	MSG_FSM_CMD,
	MSG_FSM_DATA,
};

struct mtxorb_hndl {
	enum msg_fsm_states msg_fsm;
	/* Build up the msg. */
	struct proto_cmd_data msg;
	uint8_t msg_data_left;
};

static const uint8_t mtxorb_cmds[PROTO_CMD_LAST] = {
	/* CMD_INVALID catch 0x0 (an thus undefined commands of this list) */
	[PROTO_CMD_INVALID] = 0x0,
	[PROTO_CMD_AUTO_LINE_WRAP_ON] = 0x43,
	[PROTO_CMD_AUTO_LINE_WRAP_OFF] = 0x44,
	[PROTO_CMD_AUTO_SCROLL_ON] = 0x51,
	[PROTO_CMD_AUTO_SCROLL_OFF] = 0x52,
	[PROTO_CMD_SET_CURSOR_POS] = 0x47,
	[PROTO_CMD_SEND_CURSOR_HOME] = 0x48,
	[PROTO_CMD_UNDERLINE_CURSOR_ON] = 0x4a,
	[PROTO_CMD_UNDERLINE_CURSOR_OFF] = 0x4b,
	[PROTO_CMD_BLINK_CURSOR_ON] = 0x53,
	[PROTO_CMD_BLINK_CURSOR_OFF] = 0x54,
	[PROTO_CMD_CURSOR_LEFT] = 0x4c,
	[PROTO_CMD_CURSOR_RIGHT] = 0x4d,
	[PROTO_CMD_CLR_DISPLAY] = 0x58,
	[PROTO_CMD_BLINK_CURSOR_ON] = 0x42,
	[PROTO_CMD_BLINK_CURSOR_OFF] = 0x46,
};

static int msg_fsm_run(struct mtxorb_hndl *h, uint8_t c)
{
	int ret = 0;
	enum proto_cmds cmd;
	switch(h->msg_fsm) {
	case MSG_FSM_NONE:
		h->msg.cmd = PROTO_CMD_INVALID;
		if (c == MTXORB_HEADER) {
			memset(&h->msg, 0, sizeof(h->msg));
			h->msg_fsm = MSG_FSM_HEADER;
		}
		break;
	case MSG_FSM_HEADER:
		h->msg.cmd = PROTO_CMD_INVALID;
		for (cmd = PROTO_CMD_FIRST; cmd <= PROTO_CMD_LAST; cmd++) {
			if (mtxorb_cmds[cmd] == c) {
				h->msg.cmd = cmd;
				break;
			}
		}
		if (h->msg.cmd == PROTO_CMD_SET_CURSOR_POS) {
			h->msg_fsm = MSG_FSM_CMD;
			h->msg_data_left = 2;
			break;
		}
		/* Reset the fsm in case the command is invalid or no more bytes
		 * are expected (message completed).
		 */
		h->msg_fsm = MSG_FSM_NONE;
		break;
	case MSG_FSM_CMD:
		h->msg_fsm = MSG_FSM_DATA;
	case MSG_FSM_DATA:
		if (h->msg.cmd == PROTO_CMD_SET_CURSOR_POS) {
			if (h->msg_data_left == 2)
				h->msg.data.pos.col = c;
			else
				h->msg.data.pos.row = c;
			h->msg_data_left--;
			if (!h->msg_data_left)
				h->msg_fsm = MSG_FSM_NONE;
		}
		break;
	}
	return ret;
}

static int mtxorb_parse_cmd(void *hndl, struct circ_buf *b, int b_size, struct proto_cmd_data *d)
{
	struct mtxorb_hndl *h = hndl;
	while (CIRC_CNT(b->head, b->tail, b_size) >= 1) {
		uint8_t cmd = PROTO_CMD_FIRST;
		uint8_t c = b->buf[b->tail];
		b->tail = ((b->tail + 1) & (b_size - 1));
		msg_fsm_run(h, c);
		if (h->msg_fsm == MSG_FSM_NONE && h->msg.cmd != PROTO_CMD_INVALID) {
			*d = h->msg;
			return 1;
		}
		/* On invalid command or incomplete message, continue to parse new bytes */
	}

	if (h->msg_fsm != MSG_FSM_NONE) {
		if (h->msg.cmd == PROTO_CMD_INVALID)
			return -1;
		return 1;
	}
	return 0;

}

int mtxorb_init(struct mtxorb_hndl **hndl, struct proto_cmd_ops *ops)
{
	struct mtxorb_hndl *p;
	/* malloc etc.. */
	*hndl = calloc(1, sizeof(struct mtxorb_hndl));
	if (!*hndl) {
		return -1;
	}
	p = *hndl;

	ops->parse_cmd = mtxorb_parse_cmd;
	p->msg_fsm = MSG_FSM_NONE;
	return 0;
}

int mtxorb_deinit(struct mtxorb_hndl *hndl)
{
	if (hndl)
		free(hndl);
	return 0;
}
