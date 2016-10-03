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

#define SERVER_PATH "tpf_unix_sock.server"
#define CLIENT_PATH "tpf_unix_sock.client"
#define DATA "Hello from client\n"

int main(void)
{
	int client_sd, rc, addr_len, bytes_recv;
	struct sockaddr_un cli_addr, srv_addr, peer_addr;
	char buf[256];

	memset(&srv_addr, 0, sizeof(struct sockaddr_un));
	memset(&cli_addr, 0, sizeof(struct sockaddr_un));
	client_sd = socket(AF_UNIX, SOCK_DGRAM, 0);
	if (client_sd == -1) {
		perror("SOCKET ERROR\n");
		exit(1);
	}

	cli_addr.sun_family = AF_UNIX;
	strcpy(cli_addr.sun_path, CLIENT_PATH);
	addr_len = sizeof(cli_addr);
	unlink(CLIENT_PATH);
	rc = bind(client_sd, (struct sockaddr *) &cli_addr, addr_len);
	if (rc == -1){
		perror("Client BIND ERROR\n");
		close(client_sd);
		exit(1);
	}

	srv_addr.sun_family = AF_UNIX;
	strcpy(srv_addr.sun_path, SERVER_PATH);

	strcpy(buf, DATA);
	printf("Sending data...\n");
	rc = sendto(client_sd, buf, strlen(buf), 0, (struct sockaddr *) &srv_addr, sizeof(srv_addr));
	if (rc == -1) {
		perror("SENDTO ERROR\n");
		close(client_sd);
		exit(1);
	}
	else {
		printf("Data sent!\n");
	}

	addr_len = sizeof(struct sockaddr_un);
	bytes_recv = recvfrom(client_sd, buf, sizeof(buf), 0,
			      (struct sockaddr *) &(peer_addr),
			      &addr_len);
	if (bytes_recv == -1){
		perror("RECVFROM ERROR\n");
		close(client_sd);
		exit(1);
	}
	printf("%.*s\n", bytes_recv, buf);

	close(client_sd);
	return 0;
}
