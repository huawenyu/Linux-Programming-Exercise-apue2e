/** This program blocks SIGTERM signal for 10 seconds using sigprocmask(2)
 * After that the signal is unblocked and the queued signal is handled.
 */
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static int got_signal = 0;

static void hdl (int sig)
{
	got_signal = 1;
}

int main (int argc, char *argv[])
{
	sigset_t mask;
	sigset_t orig_mask;
	struct sigaction act;

	memset (&act, 0, sizeof(act));
	act.sa_handler = hdl;

	if (sigaction(SIGTERM, &act, 0)) {
		perror ("sigaction");
		return 1;
	}

	sigemptyset (&mask);
	sigaddset (&mask, SIGTERM);

	if (sigprocmask(SIG_BLOCK, &mask, &orig_mask) < 0) {
		perror ("sigprocmask");
		return 1;
	}

	sleep (10);

	if (sigprocmask(SIG_SETMASK, &orig_mask, NULL) < 0) {
		perror ("sigprocmask");
		return 1;
	}

	sleep (1);

	if (got_signal)
		puts ("Got signal");

	return 0;
}
