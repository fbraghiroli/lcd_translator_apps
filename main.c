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

#define error(args...) fprintf(stderr, ##args)

static int set_interface_attribs(int fd, int speed)
{
        struct termios tty;
        if (tcgetattr (fd, &tty) != 0)
		return -errno;

        cfsetospeed (&tty, speed);
        cfsetispeed (&tty, speed);

	tty.c_cflag |= CLOCAL | CREAD;
	tty.c_cflag &= ~CSIZE;
	tty.c_cflag |= CS8;         /* 8-bit characters */
	tty.c_cflag &= ~PARENB;     /* no parity bit */
	tty.c_cflag &= ~CSTOPB;     /* only need 1 stop bit */
	tty.c_cflag &= ~CRTSCTS;    /* no hardware flowcontrol */

	tty.c_lflag |= ICANON | ISIG;  /* canonical input */
	tty.c_lflag &= ~(ECHO | ECHOE | ECHONL | IEXTEN);

	tty.c_iflag &= ~IGNCR;  /* preserve carriage return */
	tty.c_iflag &= ~INPCK;
	tty.c_iflag &= ~(INLCR | ICRNL | IUCLC | IMAXBEL);
	tty.c_iflag &= ~(IXON | IXOFF | IXANY);   /* no SW flowcontrol */

	tty.c_oflag &= ~OPOST;

        if (tcsetattr (fd, TCSANOW, &tty) != 0)
		return -errno;

        return 0;
}

/*
socat -d -d pty,rawer,echo=0 pty,rawer,echo=0
*/

int main(int argc, char *argv[])
{
	char *server_port = "/dev/ttyUSB0";
	char *client_port = "/dev/ttyUSB1";
	int fd_server = -1, fd_client = -1;

	/* TODO: use getopt */
	/* <app> [server port] [client port] */
	if (argv[1])
		server_port = argv[1];
	if (argv[2])
		client_port = argv[2];

	fd_server = open(server_port, O_RDWR | O_NOCTTY | O_SYNC);
	if (fd_server < 0) {
		error("Failed to open server port: %s\n", server_port);
		goto exit_open;
	}
	set_interface_attribs(fd_server, B115200);

	fd_client = open(client_port, O_RDWR | O_NOCTTY | O_SYNC);
	if (fd_client < 0) {
		error("Failed to open client port: %s\n", client_port);
		goto exit_open;
	}
	set_interface_attribs(fd_client, B115200);

exit_open:
	if (fd_server >= 0)
		close(fd_server);
	if (fd_client >= 0)
		close(fd_client);

	return 0;
}
