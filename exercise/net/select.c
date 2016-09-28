/* sample from:
 *   https://www.gnu.org/software/libc/manual/html_node/Server-Example.html
 *
 *   Show how the network lib function select() work on the server side.
 */

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#define PORT    5555
#define MAXMSG  512

int read_from_client (int filedes)
{
	char buffer[MAXMSG];
	int nbytes;

	nbytes = read (filedes, buffer, MAXMSG);
	if (nbytes < 0) {
		/* Read error. */
		perror ("read");
		exit (EXIT_FAILURE);
	}
	else if (nbytes == 0) {
		/* End-of-file. */
		return -1;
	}
	else {
		/* Data read. */
		fprintf (stderr, "Server: got message: `%s'\n", buffer);
		return 0;
	}
}

int main (void)
{
	int sock;
	fd_set active_fd_set, read_fd_set;
	int i;
	struct sockaddr_in srv_addr;
	struct sockaddr_in cli_addr;
	size_t size;

	/* Create the socket. */
	sock = socket (PF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		perror ("socket");
		exit (EXIT_FAILURE);
	}
	/* Give the socket a srv_addr. */
	srv_addr.sin_family = AF_INET;
	srv_addr.sin_port = htons (PORT);
	srv_addr.sin_addr.s_addr = htonl (INADDR_ANY);
	if (bind (sock, (struct sockaddr *) &srv_addr, sizeof (srv_addr)) < 0) {
		perror ("bind");
		exit (EXIT_FAILURE);
	}

	/* set it up to accept connections. */
	if (listen (sock, 1) < 0) {
		perror ("listen");
		exit (EXIT_FAILURE);
	}

	/* Initialize the set of active sockets. */
	FD_ZERO (&active_fd_set);
	FD_SET (sock, &active_fd_set);

	for (;;) {
		/* Block until input arrives on one or more active sockets. */
		read_fd_set = active_fd_set;
		if (select (FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0) {
			perror ("select");
			exit (EXIT_FAILURE);
		}

		/* Service all the sockets with input pending. */
		for (i = 0; i < FD_SETSIZE; ++i) {
			if (FD_ISSET (i, &read_fd_set)) {
				if (i == sock) {
					/* Connection request on original socket. */
					int new;
					size = sizeof (cli_addr);
					new = accept (sock,
						      (struct sockaddr *) &cli_addr,
						      (socklen_t *)&size);
					if (new < 0) {
						perror ("accept");
						exit (EXIT_FAILURE);
					}
					fprintf (stderr,
						 "Server: connect from host %s, port %hd.\n",
						 inet_ntoa (cli_addr.sin_addr),
						 ntohs (cli_addr.sin_port));
					FD_SET (new, &active_fd_set);
				}
				else {
					/* Data arriving on an already-connected socket. */
					if (read_from_client (i) < 0) {
						close (i);
						FD_CLR (i, &active_fd_set);
					}
				}
			}
		}
	}
	return 0;
}

