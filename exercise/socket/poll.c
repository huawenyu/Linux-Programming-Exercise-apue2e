#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <errno.h>

#define SERVER_PORT  12345

int main (int argc, char *argv[])
{
	int i, j, len, rc, on = 1;
	int listen_sd = -1, new_sd = -1;
	int desc_ready, end_server = 0, compress_array = 0;
	int close_conn;
	char buffer[80];
	struct sockaddr_in addr;
	int timeout;
	struct pollfd fds[200];
	int nfds = 1;

	listen_sd = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sd < 0) {
		perror("socket() failed");
		exit(-1);
	}

	/* Allow socket descriptor to be reuseable */
	rc = setsockopt(listen_sd, SOL_SOCKET,  SO_REUSEADDR,
			(char *)&on, sizeof(on));
	if (rc < 0) {
		perror("setsockopt() failed");
		close(listen_sd);
		exit(-1);
	}

	/* Set socket to be nonblocking.
	   All of the sockets for the incoming connections will also be nonblocking
	   since they will inherit that state from the listening socket. */
	rc = ioctl(listen_sd, FIONBIO, (char *)&on);
	if (rc < 0) {
		perror("ioctl() failed");
		close(listen_sd);
		exit(-1);
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(SERVER_PORT);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	rc = bind(listen_sd, (struct sockaddr *)&addr, sizeof(addr));
	if (rc < 0) {
		perror("bind() failed");
		close(listen_sd);
		exit(-1);
	}

	rc = listen(listen_sd, 32);
	if (rc < 0) {
		perror("listen() failed");
		close(listen_sd);
		exit(-1);
	}

	memset(fds, 0 , sizeof(fds));
	fds[0].fd = listen_sd;
	fds[0].events = POLLIN;

	/* milliseconds precise */
	timeout = (3 * 60 * 1000);

	/* - Loop waiting for incoming connects
	   - or for incoming data on any of the connected sockets. */
	while (!end_server) {
		printf("Waiting on poll()...\n");
		rc = poll(fds, nfds, timeout);
		if (rc < 0) {
			perror("  poll() failed");
			break;
		}
		/* timeout */
		else if (rc == 0) {
			printf("  poll() timed out.  End program.\n");
			break;
		}

		/* POLLIN */
		for (i = 0; i < nfds; i++) {
			if (fds[i].revents == 0)
				continue;
			/* If revents is not POLLIN, it's an unexpected result,  */
			if (fds[i].revents != POLLIN) {
				printf("  Error! revents = %d\n", fds[i].revents);
				end_server = 1;
				break;

			}
			/* the listening socket */
			if (fds[i].fd == listen_sd) {
				printf("  Listening socket is readable\n");
				/* Accept all incoming connections which queued up on the listening socket */
				do {
					/* Accept each incoming connection.
					     - If accept fails with EWOULDBLOCK, then we have accepted all of them.
					     - Any other failure on accept will cause us to end the server. */
					new_sd = accept(listen_sd, NULL, NULL);
					if (new_sd < 0) {
						if (errno == EWOULDBLOCK)
							break;
						perror("  accept() failed");
						end_server = 1;
					}
					else {
						/* Add the new incoming socket to the pollfd structure */
						printf("  New incoming connection - %d\n", new_sd);
						fds[nfds].fd = new_sd;
						fds[nfds].events = POLLIN;
						nfds++;
					}
				} while (new_sd != -1);
			}
			else {
				printf("  All incomming connection %d is readable\n", fds[i].fd);
				close_conn = 0;
				/* Receive all incoming data on this socket */
				for (;;) {
					/* - the recv fails with EWOULDBLOCK means data over
					   - If any other failure occurs, we will close the connection. */
					rc = recv(fds[i].fd, buffer, sizeof(buffer), 0);
					if (rc < 0) {
						/* all buffered data wad received */
						if (errno == EWOULDBLOCK)
							break;
						perror("  recv() failed");
						close_conn = 1;
					}
					/* connection closed by the remote client */
					else if (rc == 0) {
						printf("  Connection closed\n");
						close_conn = 1;
						break;
					}

					/* process the received data (maybe partial) */
					len = rc;
					printf("  %d bytes received\n", len);
					/* Echo the data back to the client */
					rc = send(fds[i].fd, buffer, len, 0);
					if (rc < 0) {
						perror("  send() failed");
						close_conn = 1;
						break;
					}
				}

				/* clean up the descriptor. */
				if (close_conn) {
					close(fds[i].fd);
					fds[i].fd = -1;
					compress_array = 1;
				}
			}
		}

		/* Squeeze array and decrement the number of file descriptors,
		   the events field is same and revents is output.          */
		if (!compress_array)
			continue;
		compress_array = 0;
		for (i = 0; i < nfds; i++) {
			if (fds[i].fd != -1)
				continue;
			for(j = i; j < nfds; j++) {
				fds[j].fd = fds[j+1].fd;
			}
			nfds--;
		}
	}

	/* Clean up all of the sockets that are open */
	for (i = 0; i < nfds; i++) {
		if(fds[i].fd < 0)
			continue;
		close(fds[i].fd);
	}
}
