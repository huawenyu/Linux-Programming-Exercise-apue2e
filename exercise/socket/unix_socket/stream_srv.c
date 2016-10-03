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

#define SOCK_PATH  "tpf_unix_sock.server"
#define DATA "Hello from server"

int main(void)
{
	int server_sock, client_sock, len, rc;
	int bytes_rec = 0;
	struct sockaddr_un server_sockaddr;
	struct sockaddr_un client_sockaddr;
	char buf[256];
	int backlog = 10;

	memset(&server_sockaddr, 0, sizeof(struct sockaddr_un));
	memset(&client_sockaddr, 0, sizeof(struct sockaddr_un));
	memset(buf, 0, 256);

	server_sock = socket(AF_UNIX, SOCK_STREAM, 0);
	if (server_sock == -1){
		perror("SOCKET ERROR\n");
		exit(1);
	}

	/* Unlink the file so the bind will succeed. */
	server_sockaddr.sun_family = AF_UNIX;
	strcpy(server_sockaddr.sun_path, SOCK_PATH);
	len = sizeof(server_sockaddr);

	unlink(SOCK_PATH);
	rc = bind(server_sock, (struct sockaddr *) &server_sockaddr, len);
	if (rc == -1){
		perror("BIND ERROR\n");
		close(server_sock);
		exit(1);
	}

	/* Listen for any client sockets */
	rc = listen(server_sock, backlog);
	if (rc == -1){
		perror("LISTEN ERROR\n");
		close(server_sock);
		exit(1);
	}
	printf("socket listening...\n");

	client_sock = accept(server_sock, (struct sockaddr *) &client_sockaddr, &len);
	if (client_sock == -1){
		perror("ACCEPT ERROR\n");
		close(server_sock);
		close(client_sock);
		exit(1);
	}

	len = sizeof(client_sockaddr);
	rc = getpeername(client_sock, (struct sockaddr *) &client_sockaddr, &len);
	if (rc == -1){
		perror("GETPEERNAME ERROR\n");
		close(server_sock);
		close(client_sock);
		exit(1);
	}
	else {
		printf("Client socket filepath: %s\n", client_sockaddr.sun_path);
	}

	printf("waiting to read...\n");
	bytes_rec = recv(client_sock, buf, sizeof(buf), 0);
	if (bytes_rec == -1){
		perror("RECV ERROR\n");
		close(server_sock);
		close(client_sock);
		exit(1);
	}
	else {
		printf("DATA RECEIVED = %s\n", buf);
	}

	memset(buf, 0, 256);
	strcpy(buf, DATA);
	printf("Sending data...\n");
	rc = send(client_sock, buf, strlen(buf), 0);
	if (rc == -1) {
		perror("SEND ERROR: %d");
		close(server_sock);
		close(client_sock);
		exit(1);
	}
	else {
		printf("Data sent!\n");
	}

	close(server_sock);
	close(client_sock);
	return 0;
}
