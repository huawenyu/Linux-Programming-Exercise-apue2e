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
#define DATA "Hello from client"

int main(void)
{
	int client_sock, rc, len;
	struct sockaddr_un server_sockaddr;
	struct sockaddr_un client_sockaddr;
	char buf[256];

	memset(&server_sockaddr, 0, sizeof(struct sockaddr_un));
	memset(&client_sockaddr, 0, sizeof(struct sockaddr_un));

	client_sock = socket(AF_UNIX, SOCK_STREAM, 0);
	if (client_sock == -1) {
		perror("SOCKET ERROR\n");
		exit(1);
	}

	/* Unlink the file so the bind will succeed. */
	client_sockaddr.sun_family = AF_UNIX;
	strcpy(client_sockaddr.sun_path, CLIENT_PATH);
	len = sizeof(client_sockaddr);

	unlink(CLIENT_PATH);
	rc = bind(client_sock, (struct sockaddr *) &client_sockaddr, len);
	if (rc == -1){
		perror("BIND ERROR\n");
		close(client_sock);
		exit(1);
	}

	server_sockaddr.sun_family = AF_UNIX;
	strcpy(server_sockaddr.sun_path, SERVER_PATH);
	rc = connect(client_sock, (struct sockaddr *) &server_sockaddr, len);
	if(rc == -1){
		perror("CONNECT ERROR\n");
		close(client_sock);
		exit(1);
	}

	strcpy(buf, DATA);
	printf("Sending data...\n");
	rc = send(client_sock, buf, strlen(buf), 0);
	if (rc == -1) {
		perror("SEND ERROR\n");
		close(client_sock);
		exit(1);
	}
	else {
		printf("Data sent!\n");
	}

	printf("Waiting to recieve data...\n");
	memset(buf, 0, sizeof(buf));
	rc = recv(client_sock, buf, sizeof(buf), 0);
	if (rc == -1) {
		perror("RECV ERROR\n");
		close(client_sock);
		exit(1);
	}
	else {
		printf("DATA RECEIVED = %s\n", buf);
	}

	close(client_sock);
	return 0;
}
