/* test_write.c
 *
 * $Id: test_write.c,v 1.6 2009/03/09 17:59:23 mjona Exp $
 *
 * LCD driver for HD44780 compatible displays connected to the parallel port.
 *
 * How to write to the device.
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
	int fd, i;
	unsigned char *buffer = calloc(argc-2, sizeof(unsigned char));

	if (argc < 3) {
		fprintf(stderr,"Usage: %s <offset> <byte1> [<byte2> ....]\n"
			"\tWrite <byte1> <byte2> .... to /dev/lcd\n"
			"\tstarting at <offset>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	if ((fd = open("/dev/lcd", O_WRONLY)) == -1) {
		perror("/dev/lcd");
		exit(EXIT_FAILURE);
	}

	if (lseek(fd, atoi(argv[1]), SEEK_SET) == (off_t)-1) {
		perror("lseek");
		exit(EXIT_FAILURE);
	}

	for (i = 2; i < argc; ++i)
		buffer[i-2] = (unsigned char)(atoi(argv[i]) & 0xff);

	if (write(fd, buffer, argc-2) == (ssize_t)-1) {
		perror("write");
		exit(EXIT_FAILURE);
	}

	if (close(fd) == -1) {
		perror("close");
		exit(EXIT_FAILURE);
	}

	free(buffer);

	exit(EXIT_SUCCESS);
}
