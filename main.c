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

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <ctype.h>
#include "utils.h"
#include "proto.h"
#include "circ_buf.h"
#include "ctrl_slcd.h"

#define BUF_SIZE (1 << 10) /* Must be power of 2 */

struct cfg_params {
	char *server_port;
	char *client_port;
};

static int init_server(int *fd_server, const struct cfg_params *cfg)
{
	/* On NuttX, server tty might be available after this program tries to open
	 * the device (usb enumeration).
	 * Just busy waiting for the device.
	 */
	int open_retry = 10;
	do {
		*fd_server = open(cfg->server_port, O_RDWR /*| O_NOCTTY | O_SYNC*/);
		sleep(1);
	} while(open_retry-- && *fd_server < 0);
	if (*fd_server < 0) {
		error("Failed to open server port: %s\n", cfg->server_port);
		return -errno;
	}
	tty_set_attribs(*fd_server, B19200);
	return 0;
}

static int init_client(int *fd_client, const struct cfg_params *cfg)
{
/* NuttX hardfault if it is not a tty! */
	*fd_client = open(cfg->client_port, O_RDWR /*| O_NOCTTY | O_SYNC*/);
	if (*fd_client < 0) {
		error("Failed to open client port: %s\n", cfg->client_port);
		return -errno;
	}
	tty_set_attribs(*fd_client, B19200);
	return 0;
}

/*
socat -d -d pty,rawer,echo=0 pty,rawer,echo=0
socat -d -d pty,rawer,echo=0,link=/tmp/pts0 pty,rawer,echo=0,link=/tmp/pts1
*/

/* Linux only */
int main(int argc, char *argv[])
{
	static struct cfg_params cfg;
	cfg.server_port = "/dev/ttyACM0";
	cfg.client_port = "/dev/null";
	int fd_server = -1, fd_client = -1;
	struct mtxorb_hndl *mtxorb;
	struct proto_cmd_ops mtxorb_ops;

	/* TODO: use getopt */
	/* <app> [server port] [client port] */
	if (argv[1])
		cfg.server_port = argv[1];
	if (argv[2])
		cfg.client_port = argv[2];

	if (init_server(&fd_server, &cfg) < 0)
		goto exit_init;

	if (init_client(&fd_client, &cfg) < 0)
		goto exit_init;

	if (proto_mtxorb_init(&mtxorb, &mtxorb_ops) < 0)
		goto exit_init;

	while (1) {
		char c;
		int rret;
		struct proto_cmd_data cdata;
		/* Assume to have a blocking read */
		rret = read(fd_server, &c , 1);
		if (rret < 0) {
			if (errno == EINTR) {
				break;
			}
			error("read error: %d\n", -errno);
			usleep(200*1000);
		}
		if (!rret) {
			info("eof\n");
			break;
		}
#if 1
		if (mtxorb_ops.parse_cmd(mtxorb, c, &cdata) == 1) {
			if (cdata.cmd == PROTO_CMD_ASCII) {
				if (isprint(cdata.data.ascii) || cdata.data.ascii == 0xa)
					printf("%c", (unsigned char)cdata.data.ascii);
				else
					printf("\n0x%02x\n", (unsigned char)cdata.data.ascii);
			} else {
				printf("\nCMD: %s", proto_cmds_str[cdata.cmd]);
				if (cdata.cmd == PROTO_CMD_SET_CURSOR_POS)
					printf("[r: %02d c: %02d]", cdata.data.pos.row, cdata.data.pos.col);
				printf("\n");
			}
		}
#endif

#if 0
		if (isprint(c) || c == 0xa)
			printf("%c", (unsigned char)c);
		else
			printf("\n0x%02x\n", (unsigned char)c);
#endif

	};

exit_init:
	proto_mtxorb_deinit(mtxorb);

	if (fd_server >= 0)
		close(fd_server);
	if (fd_client >= 0)
		close(fd_client);

	return 0;
}

/* NuttX entry point */
int lcd_translator_main(int argc, char *argv[])
{
	//while (1) {
	//	printf("ciao\n");
	//	sleep(1);
	//};
	//sleep(5);

	static struct cfg_params cfg;
	cfg.server_port = "/dev/ttyACM0";
	cfg.client_port = "/dev/slcd0";
	int fd_server = -1;
	struct mtxorb_hndl *mtxorb;
	struct proto_cmd_ops mtxorb_ops;
	struct ctrl_slcd *slcd = NULL;

	if (argc > 1)
		cfg.server_port = argv[1];
	if (argc > 2)
		cfg.client_port = argv[2];

	//printf("Hello\n");
	sleep(1);
	if (init_server(&fd_server, &cfg) < 0) {
		error("init_server fail\n");
		goto exit_init;
	}
	printf("init_server ok\n");
	sleep(1);
	if (!(slcd = ctrl_slcd_init(cfg.client_port))) {
		error("ctrl_slcd_init fail\n");
		goto exit_init;
	}
	printf("ctrl_slcd_init ok\n");
	sleep(1);
	if (proto_mtxorb_init(&mtxorb, &mtxorb_ops) < 0) {
		error("proto_mtxorb_init fail\n");
		goto exit_init;
	}
	printf("proto_mtxorb_init ok\n");
	sleep(1);
	while (1) {
		char c;
		int rret;
		struct proto_cmd_data cdata;
		/* Assume to have a blocking read */
		rret = read(fd_server, &c , 1);
		if (rret < 0) {
			if (errno == EINTR) {
				break;
			}
			error("read error: %d\n", -errno);
			usleep(200*1000);
			continue;
		}
		if (!rret) {
			info("eof\n");
			break;
		}

		if (mtxorb_ops.parse_cmd(mtxorb, c, &cdata) == 1) {
			ctrl_slcd_cmd(slcd, &cdata);
		}

#if 0
		if (isprint(c) || c == 0xa)
			printf("%c", (unsigned char)c);
		else
			printf("\n0x%02x\n", (unsigned char)c);
#endif

	};

exit_init:
	proto_mtxorb_deinit(mtxorb);
	ctrl_slcd_deinit(slcd);
	if (fd_server >= 0)
		close(fd_server);

	return 0;

}
