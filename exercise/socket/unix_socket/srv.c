#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>

static char *socket_path = "#hidden";

int main(int argc, char *argv[]) {
	struct sockaddr_un addr;
	char buf[100];
	int fd, cl, rc;

	if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
		perror("socket error");
		exit(-1);
	}

	/* If using abstract name, must padded the path with '\0'. */
	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path)-1);
	if (addr.sun_path[0] == '#')
		addr.sun_path[0] = '\0';

	/* Abstract Name donn't need remove the addr-path file */
	if (addr.sun_path[0] != '\0')
		unlink(socket_path);
	if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
		perror("bind error");
		exit(-1);
	}

	if (listen(fd, 32) == -1) {
		perror("listen error");
		exit(-1);
	}

	while (1) {
		if ((cl = accept(fd, NULL, NULL)) == -1) {
			perror("accept error");
			continue;
		}

		while ((rc = read(cl, buf, sizeof(buf))) > 0) {
			printf("read %u bytes: %.*s\n", rc, rc, buf);
		}
		if (rc == -1) {
			perror("read");
			exit(-1);
		}
		else if (rc == 0) {
			printf("EOF\n");
			close(cl);
		}
	}
	return 0;
}
