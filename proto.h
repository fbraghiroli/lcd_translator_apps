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

#include "circ_buf.h"

enum proto_cmds {
	PROTO_CMD_INVALID,
	/* This list have been created by referring to mtxorb datasheet,
	 * however it should be generic.
	 */
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
	PROTO_CMD_CLR_DISPLAY,
	/* skip contrast */
	PROTO_CMD_BACKLIGHT_ON,
	PROTO_CMD_BACKLIGHT_OFF,
	/* skip read module type */
	PROTO_CMD_LAST,
	PROTO_CMD_FIRST = PROTO_CMD_INVALID,
};

const char * proto_cmds_str[] = {
	[PROTO_CMD_INVALID] = "invalid",
	[PROTO_CMD_AUTO_LINE_WRAP_ON] = "auto_line_wrap_on",
	[PROTO_CMD_AUTO_LINE_WRAP_OFF] = "auto_line_wrap_off",
	[PROTO_CMD_AUTO_SCROLL_ON] = "auto_scroll_on",
	[PROTO_CMD_AUTO_SCROLL_OFF] = "auto_scroll_off",
	[PROTO_CMD_SET_CURSOR_POS] = "set_cursor_pos",
	[PROTO_CMD_SEND_CURSOR_HOME] = "send_cursor_home",
	[PROTO_CMD_UNDERLINE_CURSOR_ON] = "underline_cursor_on",
	[PROTO_CMD_UNDERLINE_CURSOR_OFF] = "underline_cursor_off",
	[PROTO_CMD_BLINK_CURSOR_ON] = "blink_cursor_on",
	[PROTO_CMD_BLINK_CURSOR_OFF] = "blink_cursor_off",
	[PROTO_CMD_CURSOR_LEFT] = "cursor_left",
	[PROTO_CMD_CURSOR_RIGHT] = "cursor_right",
	[PROTO_CMD_CLR_DISPLAY] = "clr_display",
	[PROTO_CMD_BACKLIGHT_ON] = "backlight_on",
	[PROTO_CMD_BACKLIGHT_OFF] = "backlight_off",
};

struct proto_pos {
	uint8_t col;
	uint8_t row;
};

struct proto_cmd_data {
	enum proto_cmds cmd;
	union cmd_data {
		struct proto_pos pos;
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
	int (*parse_cmd)(void *hndl, struct circ_buf *buf, int buf_size, struct proto_cmd_data *d);
};
