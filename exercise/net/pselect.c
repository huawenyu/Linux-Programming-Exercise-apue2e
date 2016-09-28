/** https://www.linuxprogrammingblog.com/code-examples/using-pselect-to-avoid-a-signal-race

  An example of a free of signal race program using sigprocmask() and pselect().
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
static volatile int exit_request = 0;

/* Signal handler. */
static void hdl (int sig)
{
	exit_request = 1;
}

/* Accept client on listening socket lfd and close the connection
 * immediatelly. */
static void handle_client (int lfd)
{
	int sock = accept(lfd, NULL, 0);
	if (sock < 0) {
		perror("accept");
		exit(1);
	}
	puts("accepted client");
	close(sock);
}

int main(int argc, char *argv[])
{
	int lfd;
	struct sockaddr_in myaddr;
	int yes = 1;
	sigset_t mask;
	sigset_t orig_mask;
	struct sigaction act;

	memset(&act, 0, sizeof(act));
	act.sa_handler = hdl;
	/* This server should shut down on SIGTERM. */
	if (sigaction(SIGTERM, &act, 0)) {
		perror ("sigaction");
		exit(1);
	}
	sigemptyset(&mask);
	sigaddset(&mask, SIGTERM);
	if (sigprocmask(SIG_BLOCK, &mask, &orig_mask) < 0) {
		perror("sigprocmask");
		exit(1);
	}

	lfd = socket(AF_INET, SOCK_STREAM, 0);
	if (lfd < 0) {
		perror ("socket");
		exit(1);
	}
	if (setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR,
		       &yes, sizeof(int)) == -1) {
		perror ("setsockopt");
		exit(1);
	}
	memset (&myaddr, 0, sizeof(myaddr));
	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = INADDR_ANY;
	myaddr.sin_port = htons (10000);
	if (bind(lfd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
		perror ("bind");
		exit(1);
	}
	if (listen(lfd, 5) < 0) {
		perror ("listen");
		exit(1);
	}

	while (!exit_request) {
		fd_set fds;
		int res;

		/* BANG! we can get SIGTERM at this point.
		   But it will be * delivered while we are in pselect(),
		   because now we block SIGTERM.
		   */

		FD_ZERO(&fds);
		FD_SET(lfd, &fds);
		res = pselect(lfd + 1, &fds, NULL, NULL, NULL, &orig_mask);
		if (res < 0 && errno != EINTR) {
			perror ("select");
			exit(1);
		}
		else if (exit_request) {
			puts ("exit");
			break;
		}
		else if (res == 0)
			continue;

		if (FD_ISSET(lfd, &fds)) {
			handle_client (lfd);
		}
	}
	return 0;
}
