/** Example of handling cases when system calls are interrupted by
 * a signal. This program copies its standard input to the standard output.
 * Here read() and write() may be interrupted. Additionally after receiving
 * a signal the program prints how much data it transfered so far.
 */
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

static volatile int print_stats = 0;

size_t total_read = 0;
size_t total_write = 0;

static void hdl (int sig)
{
	print_stats = 1;
}

static void do_print_stats (void)
{
	if (print_stats) {
		print_stats = 0;
		fprintf (stderr, "Read: %zd, Write: %zd\n", total_read, total_write);
	}
}

int main (int argc, char *argv[])
{
	ssize_t nread;
	ssize_t nwrite;
	struct sigaction act;

	memset (&act, 0, sizeof(act));

	act.sa_handler = hdl;

	if (sigaction(SIGUSR1, &act, NULL) < 0) {
		perror ("sigaction");
		return 1;
	}

	do {
		char buf[512];

		nread = read (0, buf, sizeof(buf));
		if (nread < 0) {
			if (errno != EINTR) {
				perror ("read");
				return 1;
			}

			/* We are here because the read() call was interrupted before
			 * anything was read. */
			do_print_stats ();
		}
		else if (nread == 0)
			break;
		else {
			ssize_t written = 0;
			total_read += nread;

			while (written < nread) {
				nwrite = write (1, buf + written, nread - written);
				if (nwrite < 0) {
					if (errno != EINTR) {
						perror ("write");
						return 1;
					}

					/* We are here because write() call was interrupted
					 * before anything could be written. */

				}
				else {
					written += nwrite;
					total_write += nwrite;
				}

				do_print_stats ();
			}
		}

	} while (1);

	return 0;
}
