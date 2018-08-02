/*
 * runs a proper tty on a serial port
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <pty.h>
#include <sys/wait.h>
#include <sys/fswait.h>

int main(int argc, char * argv[]) {
	int fd_master, fd_slave, fd_serial;
	char * file = "/dev/ttyS0";

	if (getuid() != 0) {
		fprintf(stderr, "%s: only root can do that\n", argv[0]);
		return 1;
	}

	if (argc > 1) {
		file = argv[1];
	}

	openpty(&fd_master, &fd_slave, NULL, NULL, NULL);
	fd_serial = open(file, O_RDWR);

	pid_t child = fork();

	if (!child) {
		dup2(fd_slave, 0);
		dup2(fd_slave, 1);
		dup2(fd_slave, 2);

		system("ttysize -q");

		char * tokens[] = {"/bin/login",NULL};
		execvp(tokens[0], tokens);
		exit(1);
	} else {

		int fds[2] = {fd_serial, fd_master};

		while (1) {
			int index = fswait2(2,fds,200);
			char buf[1024];
			int r;
			switch (index) {
				case 0: /* fd_serial */
					r = read(fd_serial, buf, 1);
					write(fd_master, buf, 1);
					break;
				case 1: /* fd_master */
					r = read(fd_master, buf, 1024);
					write(fd_serial, buf, r);
					break;
				default: /* timeout */
					{
						int result = waitpid(child, NULL, WNOHANG);
						if (result > 0) {
							/* Child login shell has returned (session ended) */
							return 0;
						}
					}
					break;
			}

		}

	}

	return 0;
}
