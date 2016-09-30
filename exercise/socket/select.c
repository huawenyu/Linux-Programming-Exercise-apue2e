#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <errno.h>

#define SERVER_PORT  12345

int main (int argc, char *argv[])
{
	int i, len, rc, on = 1;
	int listen_sd, max_sd, new_sd;
	int desc_ready, end_server = 0;
	int close_conn;
	char buffer[80];
	struct sockaddr_in addr;
	struct timeval timeout;
	struct fd_set master_set, working_set;

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

	FD_ZERO(&master_set);
	max_sd = listen_sd;
	FD_SET(listen_sd, &master_set);

	/* nanoseconds precise */
	timeout.tv_sec  = 3 * 60;
	timeout.tv_usec = 0;

	/* - Loop waiting for incoming connects
	   - or for incoming data on any of the connected sockets. */
	while (!end_server) {
		/* Copy the master fd_set over to the working fd_set.     */
		memcpy(&working_set, &master_set, sizeof(master_set));

		/* Call select() and wait 3 minutes for it to complete.   */
		printf("Waiting on select()...\n");
		rc = select(max_sd + 1, &working_set, NULL, NULL, &timeout);
		if (rc < 0) {
			perror("  select() failed");
			break;
		}
		/* timeout */
		else if (rc == 0) {
			printf("  select() timed out.  End program.\n");
			break;
		}

		/* determine which ones descriptors are readable. */
		desc_ready = rc;
		for (i=0; i <= max_sd  &&  desc_ready > 0; ++i) {
			if (!FD_ISSET(i, &working_set))
				continue;
			/* stop looking once we have found all ready descriptors */
			desc_ready -= 1;
			/* the listening socket */
			if (i == listen_sd) {
				printf("  Listening socket is readable\n");
				/* Accept all incoming connections queued up on the listening socket */
				do {
					/* Accept each incoming connection:
					   - Accept fails with EWOULDBLOCK means have accepted all of them
					   - Any other failure on accept will cause us to end the server. */
					new_sd = accept(listen_sd, NULL, NULL);
					if (new_sd < 0) {
						if (errno == EWOULDBLOCK)
							break;
						perror("  accept() failed");
						end_server = 1;
					}
					else {
						/* Add to the master read set */
						printf("  New incoming connection - %d\n", new_sd);
						FD_SET(new_sd, &master_set);
						if (new_sd > max_sd)
							max_sd = new_sd;
					}
				} while (new_sd != -1);
			}
			/* have readable client socket*/
			else {
				printf("  Descriptor %d is readable\n", i);
				close_conn = 0;
				/* Receive all incoming data on this socket */
				for (;;) {
					/* - the recv fails with EWOULDBLOCK means data over
					   - If any other failure occurs, we will close the connection. */
					rc = recv(i, buffer, sizeof(buffer), 0);
					if (rc < 0) {
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

					/* process the received (partial) data */
					len = rc;
					printf("  %d bytes received\n", len);
					/* Echo the data back to the client */
					rc = send(i, buffer, len, 0);
					if (rc < 0) {
						perror("  send() failed");
						close_conn = 1;
						break;
					}
				}

				if (!close_conn)
					continue;
				/* clean up this active connection when close_conn flag set:
				   - removing the descriptor from the master set
				   - determining the new maximum descriptor */
				close(i);
				FD_CLR(i, &master_set);
				if (i == max_sd) {
					while (!FD_ISSET(max_sd, &master_set))
						max_sd -= 1;
				}
			}
		}
	}

	/* Clean up all of the sockets that are open */
	for (i=0; i <= max_sd; ++i) {
		if (!FD_ISSET(i, &master_set))
			continue;
		close(i);
	}
}