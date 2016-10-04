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

static int recv_fd(int socket)
{
	int sent_fd, available_ancillary_element_buffer_space;
	struct msghdr socket_message;
	struct iovec io_vector[1];
	struct cmsghdr *control_message = NULL;
	char message_buffer[1];
	char ancillary_element_buffer[CMSG_SPACE(sizeof(int))];

	/* start clean */
	memset(&socket_message, 0, sizeof(struct msghdr));
	memset(ancillary_element_buffer, 0, CMSG_SPACE(sizeof(int)));

	/* setup a place to fill in message contents */
	io_vector[0].iov_base = message_buffer;
	io_vector[0].iov_len = 1;
	socket_message.msg_iov = io_vector;
	socket_message.msg_iovlen = 1;

	/* provide space for the ancillary data */
	socket_message.msg_control = ancillary_element_buffer;
	socket_message.msg_controllen = CMSG_SPACE(sizeof(int));

	if(recvmsg(socket, &socket_message, MSG_CMSG_CLOEXEC) < 0)
		return -1;

	if(message_buffer[0] != 'F') {
		/* this did not originate from the above function */
		return -1;
	}

	if((socket_message.msg_flags & MSG_CTRUNC) == MSG_CTRUNC) {
		/* we did not provide enough space for the ancillary element array */
		return -1;
	}

	/* iterate ancillary elements */
	for(control_message = CMSG_FIRSTHDR(&socket_message);
	    control_message != NULL;
	    control_message = CMSG_NXTHDR(&socket_message, control_message)) {
		if( (control_message->cmsg_level == SOL_SOCKET) &&
		    (control_message->cmsg_type == SCM_RIGHTS) ) {
			sent_fd = *((int *) CMSG_DATA(control_message));
			return sent_fd;
		}
	}

	return -1;
}

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
	{
		int send_fd;
		send_fd = recv_fd(client_sock);
		if (send_fd == -1) {
			perror("RECV-FD ERROR\n");
			close(client_sock);
			exit(1);
		}
		rc = snprintf(buf, sizeof(buf),
			 "this message wad writted directly by client,"
			 "but show on server side.\n");
		write(send_fd, buf, rc);
	}

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
