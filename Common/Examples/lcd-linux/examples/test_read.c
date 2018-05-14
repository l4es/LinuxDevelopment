/* test_read.c
 *
 * $Id: test_read.c,v 1.7 2009/04/17 09:10:41 mjona Exp $
 *
 * LCD driver for HD44780 compatible displays connected to the parallel port.
 *
 * How to read from the device.
 *
 * Copyright (C) 2005 - 2009  Mattia Jona-Lasinio (mjona@users.sourceforge.net)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char **argv)
{
	int fd, i, bytes, rbytes;
	unsigned char buffer[256];

	if (argc-3) {
		fprintf(stderr, "Usage: %s <offset> <length>\n"
			"\tRead <length> bytes from /dev/lcd\n"
			"\tstarting at <offset>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	bytes = atoi(argv[2]);

	if ((fd = open("/dev/lcd", O_RDONLY)) == -1) {
		perror("/dev/lcd");
		exit(EXIT_FAILURE);
	}

	if (lseek(fd, atoi(argv[1]), SEEK_SET) == (off_t)-1) {
		perror("lseek");
		exit(EXIT_FAILURE);
	}

	if ((rbytes = read(fd, buffer, bytes)) == (ssize_t)-1) {
		perror("read");
		exit(EXIT_FAILURE);
	}

	for (i = 0; i < rbytes; ++i)
		printf("%1$u\t'%1$c'\n", buffer[i]);

	if (close(fd) == -1) {
		perror("close");
		exit(EXIT_FAILURE);
	}

	exit(EXIT_SUCCESS);
}
