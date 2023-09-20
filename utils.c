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
#include <termios.h>
#include <unistd.h>
#include <ctype.h>
#include "utils.h"

int tty_set_attribs(int fd, int speed)
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
