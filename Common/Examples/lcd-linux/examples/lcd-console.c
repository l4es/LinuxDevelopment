/* lcd-console.c
 *
 * $Id: lcd-console.c,v 1.9 2010/01/03 00:47:55 mjona Exp $
 *
 * Uses LCD-Linux to display the console screen.
 *
 * Copyright (C) 2006 - 2009  Mattia Jona-Lasinio (mjona@users.sourceforge.net)
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <signal.h>
#include <errno.h>


#define LCD_LINUX_MAIN
#include <linux/lcd-linux.h>

#define REFRESH_RATE	150	/* Refresh rate in milliseconds */
#define PIDFILE		"/var/run/lcd-console.pid"

/* If you want lcd-console to behave as a daemon
 * uncomment the following line.
 */
//#define LCD_CONSOLE_DAEMON

#ifdef LCD_CONSOLE_DAEMON
void daemonize(void)
{
	pid_t i;
	int fd;

	/* Step 1: fork() so that the parent can exit */
	if ((i = fork()) < 0) {
		perror("fork(#1)");
		exit(1);
	}
	if (i != 0)
		exit(0);

	/* Step 2: setsid() to become a process group and session group leader */
	setsid();

	/* Step 3: fork() again so the parent (the session group leader) can exit */
	if ((i = fork()) < 0) {
		perror("fork(#2)");
		exit(1);
	}
	if (i != 0)
		exit(0);

	/* Step 4: chdir("/") to ensure that our process doesn't keep any directory in use */
	if (chdir("/") != 0) {
		perror("chdir");
		exit(1);
	}

	/* Step 5: umask(0) so that we have complete control over the permissions of anything we write */
	umask(0);

	/* Step 6: Establish new open descriptors for stdin, stdout and stderr */
	/* detach stdin */
	if (freopen("/dev/null", "r", stdin) == NULL) {
		perror("freopen");
		exit(1);
	}

	/* detach stdout and stderr */
	if ((fd = open("/dev/null", O_WRONLY, 0666)) == -1) {
		perror("open");
		exit(1);
	}
	fflush(stdout);
	dup2(fd, STDOUT_FILENO);
	fflush(stderr);
	dup2(fd, STDERR_FILENO);
	close(fd);
}
#endif

/* pid_init() and pid_exit() are taken
 * from the lcd4linux project.
 */
int pid_init(const char *pidfile)
{
	char tmpfile[256];
	char buffer[16];
	int fd, len, pid;

	snprintf(tmpfile, sizeof(tmpfile), "%s.%s", pidfile, "XXXXXX");

	if ((fd = mkstemp(tmpfile)) == -1) {
		perror("mkstemp()");
		return (-1);
	}

	if (fchmod(fd, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH) == -1) {
		perror("fchmod()");
		close(fd);
		unlink(tmpfile);
		return (-1);
	}

	snprintf(buffer, sizeof(buffer), "%d\n", (int)getpid());
	len = strlen(buffer);
	if (write(fd, buffer, len) != len) {
		perror("write()");
		close(fd);
		unlink(tmpfile);
		return (-1);
	}
	close(fd);

	while (link(tmpfile, pidfile) == -1) {

		if (errno != EEXIST) {
			perror("link()");
			unlink(tmpfile);
			return (-1);
		}

		if ((fd = open(pidfile, O_RDONLY)) == -1) {
			if (errno == ENOENT)
				continue;	/* pidfile disappared */
			perror("open()");
			unlink(tmpfile);
			return (-1);
		}

		len = read(fd, buffer, sizeof(buffer)-1);
		if (len < 0) {
			perror("read()");
			unlink(tmpfile);
			return (-1);
		}

		buffer[len] = '\0';
		if (sscanf(buffer, "%d", &pid) != 1 || pid == 0) {
			perror("scan()");
			unlink(tmpfile);
			return (-1);
		}

		if (pid == getpid()) {
			fprintf(stderr, "%s already locked by us\n", pidfile);
			unlink(tmpfile);
			return (0);
		}

		if ((kill(pid, 0) == -1) && errno == ESRCH) {
			fprintf(stderr, "removing stale PID file %s\n", pidfile);
			if (unlink(pidfile) == -1 && errno != ENOENT) {
				perror("unlink()");
				unlink(tmpfile);
				return (pid);
			}
			continue;
		}

		unlink(tmpfile);

		return (pid);
	}

	unlink(tmpfile);

	return (0);
}


int pid_exit(const char *pidfile)
{
	return (unlink(pidfile));
}

static int vcs, vcsa, lcd;
static char *buf, *disp;
static unsigned int bufsize;
static unsigned int lcd_rows, lcd_cols;
static unsigned int vs_rows, vs_cols;

void lcd_console_init(void)
{
	unsigned char tmp[2];
	struct lcd_parameters par;

	if ((vcs = open("/dev/vcs", O_RDONLY)) == -1) {
		perror("/dev/vcs");
		pid_exit(PIDFILE);
		exit(1);
	}

	if ((vcsa = open("/dev/vcsa", O_RDONLY)) == -1) {
		perror("/dev/vcsa");
		pid_exit(PIDFILE);
		exit(1);
	}
	if (lseek(vcsa, 0, SEEK_SET) == (off_t)-1) {
		perror("lseek vcsa");
		pid_exit(PIDFILE);
		exit(1);
	}
	if (read(vcsa, tmp, 2) != 2) {
		perror("read vcsa");
		pid_exit(PIDFILE);
		exit(1);
	}
	vs_rows = tmp[0];
	vs_cols = tmp[1];

	if ((lcd = open("/dev/lcd", O_RDWR)) == -1) {
		perror("/dev/lcd");
		pid_exit(PIDFILE);
		exit(1);
	}
	ioctl(lcd, LCDL_RAW_MODE, 1);
	if (ioctl(lcd, LCDL_GET_PARAM, &par) == -1) {
		perror("ioctl: GET_PARAM");
		pid_exit(PIDFILE);
		exit(1);
	}
	par.vs_rows = par.vs_cols = 0;
	lcd_rows = par.cntr_rows*par.num_cntr;
	lcd_cols = par.cntr_cols;
	bufsize = lcd_rows*lcd_cols;
	if (ioctl(lcd, LCDL_SET_PARAM, &par) == -1) {
		perror("ioctl: SET_PARAM");
		pid_exit(PIDFILE);
		exit(1);
	}

	if ((buf = (char *)malloc(2*bufsize)) == NULL) {
		perror("malloc");
		pid_exit(PIDFILE);
		exit(1);
	}
	memset(buf, ' ', 2*bufsize);
	disp = buf+bufsize;
}

void lcd_console_exit(void)
{
	free(buf);
	ioctl(lcd, LCDL_RAW_MODE, 0);
	ioctl(lcd, LCDL_CLEAR_DISP);
	close(lcd);
	close(vcsa);
	close(vcs);
}

void get_cursor_position(unsigned int *row, unsigned int *col)
{
	unsigned char tmp[2];

	if (lseek(vcsa, 2, SEEK_SET) == (off_t)-1) {
		perror("lseek vcsa");
		return;
	}
	if (read(vcsa, tmp, 2) != 2) {
		perror("read vcsa");
		return;
	}
	*col = tmp[0];
	*row = tmp[1];
}

void update_screen(void)
{
	unsigned int i, frame_base;
	unsigned int row, col;
	unsigned int frame_base_row = 0;
	unsigned int frame_base_col = 0;

	get_cursor_position(&row, &col);
	if (row >= lcd_rows)
		frame_base_row = row-(lcd_rows-1);
	if (col >= lcd_cols) {
		unsigned int tmp = lcd_cols/2;

		frame_base_col = col-(lcd_cols-1);
		if (tmp) {
			tmp = (tmp-(frame_base_col%tmp))%tmp;
			if (frame_base_col+tmp <= vs_cols-lcd_cols)
				frame_base_col += tmp;
		}
	}

	frame_base = (frame_base_row*vs_cols)+frame_base_col;

	row -= frame_base_row;
	col -= frame_base_col;
	for (i = 0; i < lcd_rows; ++i) {
		if (lseek(vcs, frame_base+(i*vs_cols), SEEK_SET) == (off_t)-1 ||
			read(vcs, buf+(i*lcd_cols), lcd_cols) != lcd_cols)
				return;
		if (i == row)
			buf[i*lcd_cols+col] = 255;
		if (strncmp(disp+(i*lcd_cols), buf+(i*lcd_cols), lcd_cols)) {
			if (lseek(lcd, i*lcd_cols, SEEK_SET) == (off_t)-1 ||
				write(lcd, buf+(i*lcd_cols), lcd_cols) != lcd_cols)
					return;
			memcpy(disp+(i*lcd_cols), buf+(i*lcd_cols), lcd_cols);
		}
	}
}

void signal_handler(int sig)
{
	if (sig == SIGALRM) {
		update_screen();
		return;
	}

	lcd_console_exit();
	pid_exit(PIDFILE);
	exit(0);
}

void install_signals(void)
{
	struct sigaction act;

	act.sa_handler = signal_handler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_RESTART;

	if (sigaction(SIGHUP, &act, NULL) == -1) {
		perror("sigaction");
		pid_exit(PIDFILE);
		exit(1);
	}
	if (sigaction(SIGINT, &act, NULL) == -1) {
		perror("sigaction");
		pid_exit(PIDFILE);
		exit(1);
	}
	if (sigaction(SIGALRM, &act, NULL) == -1) {
		perror("sigaction");
		pid_exit(PIDFILE);
		exit(1);
	}
	if (sigaction(SIGTERM, &act, NULL) == -1) {
		perror("sigaction");
		pid_exit(PIDFILE);
		exit(1);
	}
}

void set_interval_timer(void)
{
	struct itimerval tsleep = {
		{0, REFRESH_RATE*1000},
		{0, REFRESH_RATE*1000}
	};

	if (setitimer(ITIMER_REAL, &tsleep, NULL) == -1) {
		perror("setitimer");
		pid_exit(PIDFILE);
		exit(1);
	}
}

int main(int argc, char **argv)
{
#ifdef LCD_CONSOLE_DAEMON
	daemonize();
#endif
	pid_init(PIDFILE);
	lcd_console_init();
	install_signals();
	set_interval_timer();

	while (1)
		pause();

	/* We never get here */
	return (0);
}
