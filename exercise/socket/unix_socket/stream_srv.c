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
/*
#include <sys/param.h>
#include <sys/ucred.h>
*/

#define SOCK_PATH  "tpf_unix_sock.server"
#define DATA "Hello from server"

static int send_fd(int socket, int fd_to_send)
{
	struct msghdr socket_message;
	struct iovec io_vector[1];
	struct cmsghdr *control_message = NULL;
	char message_buffer[1];

	/* storage space needed for an ancillary element with a paylod of length is CMSG_SPACE(sizeof(length)) */
	char ancillary_element_buffer[CMSG_SPACE(sizeof(int))];
	int available_ancillary_element_buffer_space;

	/* at least one vector of one byte must be sent */
	message_buffer[0] = 'F';
	io_vector[0].iov_base = message_buffer;
	io_vector[0].iov_len = 1;

	/* initialize socket message */
	memset(&socket_message, 0, sizeof(struct msghdr));
	socket_message.msg_iov = io_vector;
	socket_message.msg_iovlen = 1;

	/* provide space for the ancillary data */
	available_ancillary_element_buffer_space = CMSG_SPACE(sizeof(int));
	memset(ancillary_element_buffer, 0, available_ancillary_element_buffer_space);
	socket_message.msg_control = ancillary_element_buffer;
	socket_message.msg_controllen = available_ancillary_element_buffer_space;

	/* initialize a single ancillary data element for fd passing */
	control_message = CMSG_FIRSTHDR(&socket_message);
	control_message->cmsg_level = SOL_SOCKET;
	control_message->cmsg_type = SCM_RIGHTS;
	control_message->cmsg_len = CMSG_LEN(sizeof(int));
	*((int *) CMSG_DATA(control_message)) = fd_to_send;

	return sendmsg(socket, &socket_message, 0);
}

#if 0
static void get_cred()
{
	struct ucred credentials;
	int ucred_length = sizeof(struct ucred);

	/* fill in the user data structure */
	if(getsockopt(connection_fd, SOL_SOCKET, SO_PEERCRED, &credentials, &ucred_length))
	{
		printf("could obtain credentials from unix domain socket");
		return 1;
	}

	/* the process ID of the process on the other side of the socket */
	credentials.pid;

	/* the effective UID of the process on the other side of the socket  */
	credentials.uid;

	/* the effective primary GID of the process on the other side of the socket */
	credentials.gid;

	/* To get supplemental groups, we will have to look them up in our account
	   database, after a reverse lookup on the UID to get the account name.
	   We can take this opportunity to check to see if this is a legit account.
	   */
}
#endif

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

	memset(buf, 0, sizeof(buf));
	strcpy(buf, DATA);
	printf("Sending data...\n");
	rc = send_fd(client_sock, STDOUT_FILENO);
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
