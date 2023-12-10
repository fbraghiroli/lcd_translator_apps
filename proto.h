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

#ifndef PROTO_H
#define PROTO_H

#include <stdint.h>
#include "circ_buf.h"

enum proto_cmds {
	PROTO_CMD_INVALID,
	PROTO_CMD_ASCII,
	/* This list have been created by referring to mtxorb datasheet,
	 * however it should be generic.
	 */
	PROTO_CMD_GET_SN,
	PROTO_CMD_GET_FW_VER,
	PROTO_CMD_GET_DISPLAY_TYPE,
	PROTO_CMD_AUTO_LINE_WRAP_ON,
	PROTO_CMD_AUTO_LINE_WRAP_OFF,
	PROTO_CMD_AUTO_SCROLL_ON,
	PROTO_CMD_AUTO_SCROLL_OFF,
	PROTO_CMD_SET_CURSOR_POS,      /* data */
	PROTO_CMD_SEND_CURSOR_HOME,
	PROTO_CMD_UNDERLINE_CURSOR_ON,
	PROTO_CMD_UNDERLINE_CURSOR_OFF,
	PROTO_CMD_BLINK_CURSOR_ON,
	PROTO_CMD_BLINK_CURSOR_OFF,
	PROTO_CMD_CURSOR_LEFT,
	PROTO_CMD_CURSOR_RIGHT,
	/* skip graphics / bars / etc... */
	PROTO_CMD_ADD_CUSTOM_CHAR,
	PROTO_CMD_CLR_DISPLAY,
	PROTO_CMD_SET_CONTRAST, /* data */
	PROTO_CMD_BACKLIGHT_ON, /* data */
	PROTO_CMD_BACKLIGHT_OFF,
	PROTO_CMD_BACKLIGHT_LVL, /* data */
	PROTO_CMD_GPO_OFF,
	PROTO_CMD_GPO_ON,
	/* skip read module type */
	PROTO_CMD_LAST,
	PROTO_CMD_FIRST = PROTO_CMD_INVALID,
};

extern const char *proto_cmds_str[];

struct proto_pos {
	uint8_t col;
	uint8_t row;
};

struct proto_cmd_data {
	enum proto_cmds cmd;
	union cmd_data {
		struct proto_pos pos;
		uint8_t contrast;
		uint8_t ascii;
	} data;
};

struct proto_cmd_ops {
	/* It parses only a single command at time, must be called in loop to parse all
	 * the messages.
	 * Use a circular buffer so the protocol take cares of how many bytes to parse.
	 *
	 * Return values:
	 * -1: no valid command
	 *  0: partial / incomplete command
	 *  1: valid command parsed
	 */
	int (*parse_cmd_buffered)(void *hndl, struct circ_buf *buf, int buf_size, struct proto_cmd_data *d);
	int (*parse_cmd)(void *hndl, uint8_t c, struct proto_cmd_data *d);
};

struct mtxorb_hndl;
int proto_mtxorb_init(struct mtxorb_hndl **hndl, struct proto_cmd_ops *ops);
int proto_mtxorb_deinit(struct mtxorb_hndl *hndl);

#endif //PROTO_H
