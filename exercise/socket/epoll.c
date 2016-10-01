#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <errno.h>

#define MAXEVENTS 64

static int make_socket_non_blocking (int listen_sd)
{
	int flags, s;

	flags = fcntl (listen_sd, F_GETFL, 0);
	if (flags == -1) {
		perror ("fcntl");
		return -1;
	}

	flags |= O_NONBLOCK;
	s = fcntl (listen_sd, F_SETFL, flags);
	if (s == -1) {
		perror ("fcntl");
		return -1;
	}

	return 0;
}

static int create_and_bind (char *port)
{
	struct addrinfo hints;
	struct addrinfo *result, *rp;
	int s, listen_sd;

	memset (&hints, 0, sizeof (struct addrinfo));
	hints.ai_family = AF_UNSPEC;     /* Return IPv4 and IPv6 choices */
	hints.ai_socktype = SOCK_STREAM; /* We want a TCP socket */
	hints.ai_flags = AI_PASSIVE;     /* All interfaces */

	s = getaddrinfo (NULL, port, &hints, &result);
	if (s != 0) {
		fprintf (stderr, "getaddrinfo: %s\n", gai_strerror (s));
		return -1;
	}
	for (rp = result; rp != NULL; rp = rp->ai_next) {
		listen_sd = socket (rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (listen_sd == -1)
			continue;
		s = bind (listen_sd, rp->ai_addr, rp->ai_addrlen);
		if (s == 0) {
			/* We managed to bind successfully! */
			break;
		}
		close (listen_sd);
	}
	if (rp == NULL) {
		fprintf (stderr, "Could not bind\n");
		return -1;
	}
	freeaddrinfo (result);
	return listen_sd;


	int listen_sd;

	listen_sd = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sd < 0) {
		perror("socket() failed");
		exit(-1);
	}
}

int main (int argc, char *argv[])
{
	int listen_sd, s;
	int efd;
	struct epoll_event event;
	struct epoll_event *events;

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
	   All of the sockets for the incoming connections will also be nonblocking
	   since they will inherit that state from the listening socket.
	   Maybe we don't need flag FASYNC, FD_CLOEXEC */
	rc = fcntl(listen_sd, F_SETFL,
		   fcntl(listen_sd, F_GETFL, 0) | O_NONBLOCK|FASYNC);
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

	rc = listen(listen_sd, SOMAXCONN);
	if (rc < 0) {
		perror("listen() failed");
		close(listen_sd);
		exit(-1);
	}

	/* Nowadays, this hint is no longer required, so here use epoll_create1(flags).
	   But epoll_create(size > 0) in order to ensure backward compatibility when
	   new epoll applications are run on older kernels. */
	efd = epoll_create1(0);
	if (efd == -1) {
		perror("epoll_create");
		abort();
	}

	event.data.fd = listen_sd;
	event.events = EPOLLIN | EPOLLET;
	s = epoll_ctl (efd, EPOLL_CTL_ADD, listen_sd, &event);
	if (s == -1) {
		perror ("epoll_ctl");
		abort ();
	}

	/* Buffer where events are returned */
	events = calloc (MAXEVENTS, sizeof event);

	/* The event loop */
	for (;;) {
		int n, i;

		n = epoll_wait (efd, events, MAXEVENTS, -1);
		for (i = 0; i < n; i++) {
			if ((events[i].events & EPOLLERR) ||
			    (events[i].events & EPOLLHUP) ||
			    (!(events[i].events & EPOLLIN))) {
				/* An error has occured on this fd, or the socket is not
				   ready for reading (why were we notified then?) */
				fprintf (stderr, "epoll error\n");
				close (events[i].data.fd);
				continue;
			}
			else if (listen_sd == events[i].data.fd) {
				/* We have a notification on the listening socket, which
				   means one or more incoming connections. */
				for (;;) {
					struct sockaddr in_addr;
					socklen_t in_len;
					int infd;
					char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];

					in_len = sizeof in_addr;
					infd = accept (listen_sd, &in_addr, &in_len);
					if (infd == -1) {
						if ((errno == EAGAIN) ||
						    (errno == EWOULDBLOCK)) {
							/* We have processed all incoming
							   connections. */
							break;
						}
						else {
							perror ("accept");
							break;
						}
					}
					s = getnameinfo (&in_addr, in_len,
							 hbuf, sizeof hbuf,
							 sbuf, sizeof sbuf,
							 NI_NUMERICHOST | NI_NUMERICSERV);
					if (s == 0) {
						printf("Accepted connection on descriptor %d "
						       "(host=%s, port=%s)\n", infd, hbuf, sbuf);
					}

					/* Make the incoming socket non-blocking and add it to the
					   list of fds to monitor. */
					s = make_socket_non_blocking (infd);
					if (s == -1)
						abort ();

					event.data.fd = infd;
					event.events = EPOLLIN | EPOLLET;
					s = epoll_ctl (efd, EPOLL_CTL_ADD, infd, &event);
					if (s == -1) {
						perror ("epoll_ctl");
						abort ();
					}
				}
				continue;
			}
			else {
				/* We have data on the fd waiting to be read. Read and
				   display it. We must read whatever data is available
				   completely, as we are running in edge-triggered mode
				   and won't get a notification again for the same
				   data. */
				int done = 0;

				for (;;) {
					ssize_t count;
					char buf[512];

					count = read (events[i].data.fd, buf, sizeof buf);
					if (count == -1) {
						/* If errno == EAGAIN, that means we have read all
						   data. So go back to the main loop. */
						if (errno != EAGAIN) {
							perror ("read");
							done = 1;
						}
						break;
					}
					else if (count == 0) {
						/* End of file. The remote has closed the
						   connection. */
						done = 1;
						break;
					}

					/* Write the buffer to standard output */
					s = write (1, buf, count);
					if (s == -1) {
						perror ("write");
						abort ();
					}
				}

				if (done) {
					printf ("Closed connection on descriptor %d\n",
						events[i].data.fd);

					/* Closing the descriptor will make epoll remove it
					   from the set of descriptors which are monitored. */
					close (events[i].data.fd);
				}
			}
		}
	}
	free (events);
	close (listen_sd);
	return EXIT_SUCCESS;
}
