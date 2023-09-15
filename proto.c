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

#include "proto.h"

const char * proto_cmds_str[PROTO_CMD_LAST] = {
	[PROTO_CMD_INVALID] = "invalid",
	[PROTO_CMD_ASCII] = "ascii",
	[PROTO_CMD_GET_SN] = "get_sn",
	[PROTO_CMD_GET_FW_VER] = "get_fw_ver",
	[PROTO_CMD_GET_DISPLAY_TYPE] = "get_display_type",
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
	[PROTO_CMD_ADD_CUSTOM_CHAR] = "add_custom_char",
	[PROTO_CMD_CLR_DISPLAY] = "clr_display",
	[PROTO_CMD_SET_CONTRAST] = "set_contrast",
	[PROTO_CMD_BACKLIGHT_ON] = "backlight_on",
	[PROTO_CMD_BACKLIGHT_OFF] = "backlight_off",
	[PROTO_CMD_BACKLIGHT_LVL] = "backlight_lvl",
};
