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
#include "proto.h"
#include "circ_buf.h"

#define error(args...) fprintf(stderr, ##args)
#define info(args...) fprintf(stderr, ##args)
#define BUF_SIZE (1 << 10) /* Must be power of 2 */

struct cfg_params {
	char *server_port;
	char *client_port;
};

static int set_interface_attribs(int fd, int speed)
{
        struct termios tty;
        if (tcgetattr (fd, &tty) != 0)
		return -errno;

        cfsetospeed (&tty, speed);
        cfsetispeed (&tty, speed);
	/* Reminder for serial flags:
	 * https://blog.mbedded.ninja/programming/operating-systems/linux/linux-serial-ports-using-c-cpp/
	 */
	tty.c_cflag |= CLOCAL | CREAD;
	tty.c_cflag &= ~CSIZE;
	tty.c_cflag |= CS8;         /* 8-bit characters */
	tty.c_cflag &= ~PARENB;     /* no parity bit */
	tty.c_cflag &= ~CSTOPB;     /* only need 1 stop bit */
	tty.c_cflag &= ~CRTSCTS;    /* no hardware flowcontrol */

	tty.c_lflag &= ~ICANON; /* No canonical input (no need to wait \n) */
	tty.c_lflag &= ~ISIG;  /* Disable interpretation of INTR, QUIT and SUSP */
	tty.c_lflag &= ~(ECHO | ECHOE | ECHONL);

	/* Disable any special handling of received bytes */
	tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL);
	tty.c_iflag &= ~(IXON | IXOFF | IXANY);   /* no SW flowcontrol */

	tty.c_oflag &= ~OPOST;
	tty.c_oflag &= ~ONLCR;

	tty.c_cc[VTIME] = 0; /* 0 -> blocking read */
	tty.c_cc[VMIN] = 1; /* 1 -> at least 1 byte per read */

        if (tcsetattr (fd, TCSANOW, &tty) != 0)
		return -errno;

        return 0;
}

static int init_ifaces(int *fd_server, int *fd_client, const struct cfg_params *cfg)
{
	*fd_server = open(cfg->server_port, O_RDWR /*| O_NOCTTY | O_SYNC*/);
	if (*fd_server < 0) {
		error("Failed to open server port: %s\n", cfg->server_port);
		return -errno;
	}
	set_interface_attribs(*fd_server, B19200);

	*fd_client = open(cfg->client_port, O_RDWR /*| O_NOCTTY | O_SYNC*/);
	if (*fd_client < 0) {
		error("Failed to open client port: %s\n", cfg->client_port);
		return -errno;
	}
	set_interface_attribs(*fd_client, B19200);
	return 0;
}

/*
socat -d -d pty,rawer,echo=0 pty,rawer,echo=0
socat -d -d pty,rawer,echo=0,link=/tmp/pts0 pty,rawer,echo=0,link=/tmp/pts1
*/

int main(int argc, char *argv[])
{
	static struct cfg_params cfg;
	cfg.server_port = "/dev/ttyUSB0";
	cfg.client_port = "/dev/ttyUSB1";
	int fd_server = -1, fd_client = -1;
	struct mtxorb_hndl *mtxorb;
	struct proto_cmd_ops mtxorb_ops;

	/* TODO: use getopt */
	/* <app> [server port] [client port] */
	if (argv[1])
		cfg.server_port = argv[1];
	if (argv[2])
		cfg.client_port = argv[2];

	if (init_ifaces(&fd_server, &fd_client, &cfg) < 0) {
		goto exit_init;
	}

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

		if (mtxorb_ops.parse_cmd(mtxorb, c, &cdata) == 1) {
#if 1
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
#endif
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

	if (fd_server >= 0)
		close(fd_server);
	if (fd_client >= 0)
		close(fd_client);

	return 0;
}
