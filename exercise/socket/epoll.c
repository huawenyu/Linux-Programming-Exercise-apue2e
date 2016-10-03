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
#include <sys/epoll.h>

#define MAXEVENTS 64
#define SERVER_PORT  12345

int main (int argc, char *argv[])
{
	int i, rc, on = 1;
	int efd;
	int listen_sd;
	int timeout;
	struct sockaddr_in addr;
	struct epoll_event event;
	struct epoll_event *events;
	int desc_ready;

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

	/* Nowadays, this parameter size of epoll_create(size) is no longer required,
	   But in order to ensure backward compatibility epoll_create(size > 0)
	   when new epoll applications are run on older kernels. */
	efd = epoll_create1(0);
	if (efd == -1) {
		perror("epoll_create");
		exit(-1);
	}

	event.data.fd = listen_sd;
	/* default is level-triggered, another is edge-triggered:
	   - EPOLLET edge trigger only when state (un)-readable/writable changed
	   - edge-triggered (events |= EPOLLET):
	     If we have data on the fd waiting to be read or have in-comming connections.
	     We must read whatever data is available completely,
	     as we are running in edge-triggered mode
	     and won't get a notification again for the same data.
	   event.events = EPOLLIN | EPOLLET; */
	event.events = EPOLLIN;
	rc = epoll_ctl(efd, EPOLL_CTL_ADD, listen_sd, &event);
	if (rc == -1) {
		perror("epoll_ctl");
		exit(-1);
	}

	/* Buffer where events are returned */
	events = calloc(MAXEVENTS, sizeof event);
	timeout = 3 * 60 * 1000;

	/* The event loop */
	for (;;) {
		rc = epoll_wait(efd, events, MAXEVENTS, timeout);
		if (rc < 0) {
#ifdef __linux__
			/* epoll_wait again */
			if (errno == EINTR) {
				printf("  epoll_wait() EINTR");
				continue;
			}
#endif
			perror("  epoll_wait() failed");
			break;
		}
		/* timeout */
		else if (rc == 0) {
			printf("  epoll_wait() timed out.  End program.\n");
			break;
		}

		desc_ready = rc;
		for (i = 0; i < desc_ready; i++) {
			if ((events[i].events & EPOLLERR)
				|| (events[i].events & EPOLLHUP)
				|| (!(events[i].events & EPOLLIN))) {
				/* An error has occured on this fd, or the socket is not
				   ready for reading (why were we notified then?) */
				fprintf(stderr, "epoll error\n");
				close(events[i].data.fd);
				continue;
			}
			else if (listen_sd == events[i].data.fd) {
				/* We have a notification on the listening socket, which
				   means one or more incoming connections. */
				for (;;) {
					socklen_t in_len, loc_len;
					struct sockaddr_in in_addr;
					struct sockaddr_in loc_addr;
					int new_sd;
					/* char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV]; */

					/* as input/output parameter,
					   addr_len should be calculate every times. */
					in_len = sizeof in_addr;

					new_sd = accept(listen_sd,
							(struct sockaddr *)&in_addr,
							&in_len);
					if (new_sd == -1) {
						if ((errno == EAGAIN)
							|| (errno == EWOULDBLOCK)) {
							/* We have processed all incoming
							   connections. */
							break;
						}
						/* accept again */
						else if (errno == EINTR) {
							printf("  accept() EINTR");
							continue;
						}

						perror ("  accept() failed");
						break;
					}

					/* Add to the poll set */
					getsockname(new_sd, (struct sockaddr *)&loc_addr, &loc_len);
					printf("  New incoming connection-%d %s:%d -> %s:%d\n", new_sd,
						   inet_ntoa(in_addr.sin_addr), ntohs(in_addr.sin_port),
						   inet_ntoa(loc_addr.sin_addr), ntohs(loc_addr.sin_port)
						   );
					/* another method to get printf addr
					rc = getnameinfo((struct sockaddr *)&in_addr,
							 in_len, hbuf, sizeof hbuf,
							 sbuf, sizeof sbuf,
							 NI_NUMERICHOST | NI_NUMERICSERV);
					if (rc == 0) {
						printf("Accepted connection on descriptor %d "
						       "(host=%rc, port=%rc)\n", new_sd, hbuf, sbuf);
					}
					*/

					/* Make the incoming socket non-blocking
					   and add it to the list of fds to monitor. */
					rc = fcntl(new_sd, F_SETFL,
						   fcntl(new_sd, F_GETFL, 0) | O_NONBLOCK|FASYNC);
					if (rc < 0) {
						perror("fcntl() failed");
						close(new_sd);
						exit(-1);
					}

					event.data.fd = new_sd;
					/* - EPOLLONESHOT one descriptor only trigger once, then we should add it to epoll again
					   event.events = EPOLLIN | EPOLLET | EPOLLONESHOT; */
					event.events = EPOLLIN;
					rc = epoll_ctl(efd, EPOLL_CTL_ADD, new_sd, &event);
					if (rc == -1) {
						perror("epoll_ctl");
						exit(-1);
					}
				}
				continue;
			}
			/* have readable client socket*/
			else {
				int close_conn = 0;

				for (;;) {
					ssize_t count;
					char buf[512];

					/* - recvfrom sameas recv
					     rc = recv(i, buffer, sizeof(buffer), 0); */
					count = recv(events[i].data.fd, buf, sizeof buf, 0);
					if (count == -1) {
						/* If errno == EAGAIN, that means we have read all
						   data. So go back to the main loop. */
						if (errno == EWOULDBLOCK || errno == EAGAIN) {
							printf("  recv EAGAIN or EWOULDBLOCK\n");
							break;
						}
						/* read again */
						else if (errno == EINTR) {
							printf("  recv EINTR\n");
							continue;
						}
						/* remote endpoint receive our send-data after FIN */
						else if (errno == ECONNRESET)
							printf("  recv ECONNRESET\n");
							/* fall-through close */

						perror("  recv() failed");
						close_conn = 1;
						break;
					}
					else if (count == 0) {
						/* End of file. The remote has closed the
						   connection. */
						close_conn = 1;
						break;
					}

resend_again:
					/* Write the buffer to standard output */
					rc = send(events[i].data.fd, buf, count, 0);
					if (rc < 0) {
						/* all done! */
						if (errno == EWOULDBLOCK || errno == EAGAIN)
							continue;
						else if (errno == EINTR)
							goto resend_again;
						/* Connection reset by peer. */
						else if (errno == ECONNRESET)
							; /* closed */

						perror("  send() failed");
						close_conn = 1;
						break;
					}
				}

				/* Closing the descriptor will make epoll remove it
				   from the set of descriptors which are monitored. */
				if (close_conn) {
					printf ("Closed connection on descriptor %d\n",
						events[i].data.fd);

					close (events[i].data.fd);
				}
			}
		}
	}
	free (events);
	close (listen_sd);
	return EXIT_SUCCESS;
}
