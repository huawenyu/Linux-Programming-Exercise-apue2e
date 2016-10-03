#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/select.h> /* According to POSIX.1-2001 */
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

#define SERVER_PORT  12345

enum {
	config_flag_no_read = 0,
};
static volatile sig_atomic_t l_config_flags = 0;
static void sig_hdl (int sig)
{
	if (l_config_flags & (1<<config_flag_no_read))
		l_config_flags &= ~(1<<config_flag_no_read);
	else
		l_config_flags |= 1<<config_flag_no_read;
}

static void config_flags_print(char *buff, size_t buff_len)
{
	snprintf(buff, buff_len, "flags: no-read=%d",
		 l_config_flags & (1<<config_flag_no_read) ? 1 : 0);
}

int main (int argc, char *argv[])
{
	int i, len, rc, on = 1;
	int listen_sd, max_sd, new_sd;
	int desc_ready, end_server = 0;
	int close_conn;
	char buffer[80];
	struct sockaddr_in addr;
	struct sockaddr_in cli_addr;
	int cli_addr_len;
	struct timeval timeout;
	fd_set master_set, working_set;
	struct sigaction sig_act;

	sig_act.sa_handler = &sig_hdl;
	if (sigaction(SIGUSR1, &sig_act, NULL) < 0) {
		perror ("sigaction");
		exit(-1);
	}

	listen_sd = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sd < 0) {
		perror("socket() failed");
		exit(-1);
	}

	/* Allow socket descriptor to be reuseable */
	rc = setsockopt(listen_sd, SOL_SOCKET, SO_REUSEADDR,
			(char *)&on, sizeof(on));
	if (rc < 0) {
		perror("setsockopt() failed");
		close(listen_sd);
		exit(-1);
	}

	/* Set socket to be nonblocking.
	   Maybe we don't need flag FASYNC, FD_CLOEXEC */
	rc = fcntl(listen_sd, F_SETFL,
		   fcntl(listen_sd, F_GETFL, 0) | O_NONBLOCK | FASYNC);
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
		printf("Waiting %d on %d select()...\n", getpid(), SERVER_PORT);
		rc = select(max_sd + 1, &working_set, NULL, NULL, &timeout);
		if (rc < 0) {
#ifdef __linux__
			/* select again */
			if (errno == EINTR) {
				printf("  select() EINTR");
				continue;
			}
#endif
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
					socklen_t loc_len;
					struct sockaddr_in loc_addr;

					/* as input/output parameter,
					   addr_len should be calculate every times. */
					cli_addr_len = sizeof(cli_addr);

					/* Accept each incoming connection:
					   - Accept fails with EWOULDBLOCK means have accepted all of them
					   - Any other failure on accept will cause us to end the server.

					   struct sockaddr is a generic socket address:
					   - struct sockaddr_in is the actual IPv4 address layout (it has .sin_port and .sin_addr)
					   - A UNIX domain socket will have type struct sockaddr_un */
					new_sd = accept(listen_sd,
							(struct sockaddr *)&cli_addr,
							(socklen_t *)&cli_addr_len);
					if (new_sd < 0) {
						/* The socket is marked nonblocking
						 * and no connections are present to be accepted. */
						if (errno == EWOULDBLOCK)
							break;
#ifdef __linux__
						/* same-as EWOULDBLOCK: incomming connection all done! */
						else if (errno == EAGAIN)
							break;
						/* accept again */
						else if (errno == EINTR) {
							printf("  accept() EINTR");
							continue;
						}
#endif
						perror("  accept() failed");
						end_server = 1;
					}

					/* Add to the master read set */
					getsockname(new_sd, (struct sockaddr *)&loc_addr, &loc_len);
					printf("  New incoming connection-%d %s:%d -> %s:%d\n", new_sd,
						   inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port),
						   inet_ntoa(loc_addr.sin_addr), ntohs(loc_addr.sin_port)
						   );
#ifdef __linux__
					/* Linux have no inherit from listen_sd, so set nonblocking one-by-one.
					   (Other Unix-like OS: All of the sockets for the incoming connections will also be nonblocking
					   since they will inherit that state from the listening socket.) */
					rc = fcntl(new_sd, F_SETFL,
							   fcntl(new_sd, F_GETFL, 0) | O_NONBLOCK | FASYNC);
					if (rc < 0) {
						perror("fcntl() failed");
						close(new_sd);
						exit(-1);
					}
#endif
					FD_SET(new_sd, &master_set);
					if (new_sd > max_sd)
						max_sd = new_sd;
				} while (new_sd != -1);
			}
			/* have readable client socket*/
			else {
				config_flags_print(buffer, sizeof(buffer));
				printf("  Descriptor %d is readable with %s\n",
				       i, buffer);
				close_conn = 0;
				if (l_config_flags & (1<<config_flag_no_read))
					continue;
				/* Receive all incoming data on this socket */
				for (;;) {
					/* - the recv fails with EWOULDBLOCK means data over
					   - If any other failure occurs, we will close the connection.
					   - recvfrom sameas recv
					     rc = recv(i, buffer, sizeof(buffer), 0); */
					rc = recvfrom(i, buffer, sizeof(buffer), 0, 0, 0);
					if (rc < 0) {
						/* man recv(2):
						   The socket is marked nonblocking
						   and the receive operation would block.
						   data all done!. */
						if (errno == EWOULDBLOCK) {
							printf("  recv EWOULDBLOCK\n");
							break;
						}
#ifdef __linux__
						/* same-as EWOULDBLOCK: all done! */
						else if (errno == EAGAIN) {
							printf("  recv EAGAIN\n");
							break;
						}
						/* read again */
						else if (errno == EINTR) {
							printf("  recv EINTR\n");
							continue;
						}
						/* remote endpoint receive our send-data after FIN */
						else if (errno == ECONNRESET) {
							printf("  recv ECONNRESET\n");
							continue;
						}
#endif
						perror("  recv() failed");
						close_conn = 1;
					}
					/* connection closed by the remote endpoint */
					else if (rc == 0) {
						printf("  Connection closed\n");
						close_conn = 1;
						break;
					}

					/* process the received (partial) data */
					len = rc;
					printf("  recv %d: %.*s\n", len, len, buffer);
					/* Echo the data back to the client */
#ifdef __linux__
resend_again:
#endif
					rc = send(i, buffer, len, 0);
					if (rc < 0) {
						/* all done! */
						if (errno == EWOULDBLOCK)
							continue;
#ifdef __linux__
						/* same-as EWOULDBLOCK: all done! */
						else if (errno == EAGAIN)
							continue;
						else if (errno == EINTR)
							goto resend_again;
						/* Connection reset by peer. */
						else if (errno == ECONNRESET)
							; /* closed */
#endif
						perror("  send() failed");
						close_conn = 1;
						break;
					}
				}

				/* clean up this active connection when close_conn flag set:
				   - removing the descriptor from the master set
				   - determining the new maximum descriptor */
				if (!close_conn)
					continue;
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
	return 0;
}
