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

struct ctrl_slcd;

struct ctrl_slcd* ctrl_slcd_init(const char *dev);
int ctrl_slcd_deinit(struct ctrl_slcd *hndl);
int ctrl_slcd_cmd(struct ctrl_slcd *hndl, const struct proto_cmd_data *cmd);
