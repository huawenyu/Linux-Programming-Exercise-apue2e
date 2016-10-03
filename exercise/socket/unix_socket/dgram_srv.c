#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

#define SOCK_PATH "tpf_unix_sock.server"

int main(void){

	int server_sd, addr_len, rc;
	int bytes_send, bytes_recv = 0;
	struct sockaddr_un server_sockaddr, peer_sock;
	char buf[256];
	memset(&server_sockaddr, 0, sizeof(struct sockaddr_un));
	memset(buf, 0, sizeof(buf));

	server_sd = socket(AF_UNIX, SOCK_DGRAM, 0);
	if (server_sd == -1){
		perror("SOCKET ERROR\n");
		exit(1);
	}

	server_sockaddr.sun_family = AF_UNIX;
	strcpy(server_sockaddr.sun_path, SOCK_PATH);
	addr_len = sizeof(server_sockaddr);
	unlink(SOCK_PATH);
	rc = bind(server_sd, (struct sockaddr *) &server_sockaddr, addr_len);
	if (rc == -1){
		perror("BIND ERROR\n");
		close(server_sd);
		exit(1);
	}

	for (;;) {
		printf("waiting to recvfrom...\n");
		bytes_recv = recvfrom(server_sd, buf, sizeof(buf), 0, (struct sockaddr *)&peer_sock, &addr_len);
		if (bytes_recv == -1){
			perror("RECVFROM ERROR\n");
			close(server_sd);
			exit(1);
		}
		else {
			char *p = buf;
			printf("DATA RECEIVED = %s\n", buf);
			while (*p++) {
				*p = toupper(*p);
			}
			bytes_send = sendto(server_sd, buf, bytes_recv, 0,
					    (struct sockaddr *)&peer_sock, addr_len);
		}
	}

	close(server_sd);
	return 0;
}
