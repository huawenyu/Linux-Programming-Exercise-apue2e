/* An example of a signal race condition using the traditional UNIX functions:
 * signal(), select(), accept ().
 */
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

/* Flag that tells the daemon to exit. */
static volatile sig_atomic_t exit_request = 0;

/* Signal handler. */
static void hdl (int sig)
{
	exit_request = 1;
}

/* Accept client on listening socket lfd and close the connection
 * immediatelly. */
static void handle_client (int lfd)
{
	int sock = accept (lfd, NULL, 0);
	if (sock < 0) {
		perror ("accept");
		exit (1);
	}

	puts ("accepted client");

	close (sock);
}

int main (int argc, char *argv[])
{
	int lfd;
	struct sockaddr_in myaddr;
	int yes = 1;

	/* This server should shut down on SIGTERM. */
	signal (SIGTERM, hdl);

	lfd = socket (AF_INET, SOCK_STREAM, 0);
	if (lfd < 0) {
		perror ("socket");
		return 1;
	}

	if (setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR,
		       &yes, sizeof(int)) == -1) {
		perror ("setsockopt");
		return 1;
	}

	memset (&myaddr, 0, sizeof(myaddr));
	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = INADDR_ANY;
	myaddr.sin_port = htons (10000);

	if (bind(lfd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
		perror ("bind");
		return 1;
	}

	if (listen(lfd, 5) < 0) {
		perror ("listen");
		return 1;
	}

	while (!exit_request) {
		fd_set fds;
		int res;

		/* BANG! we can get SIGTERM at this point. */

		FD_ZERO(&fds);
		FD_SET(lfd, &fds);

		res = select (lfd + 1, &fds, NULL, NULL, NULL);
		if (res < 0 && errno != EINTR) {
			perror ("select");
			return 1;
		}
		else if (exit_request) { /* EINTR */
			puts ("exit");
			break;
		}
		else if (res == 0) /* timeout */
			continue;

		if (FD_ISSET(lfd, &fds)) {
			handle_client (lfd);
		}
	}

	return 0;
}
