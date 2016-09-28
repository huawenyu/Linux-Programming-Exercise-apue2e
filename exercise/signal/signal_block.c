/*
Output:
  $ ./a.out
  Entering BLOCK state
  SIGINT signal blocked
  Leaving Blocking State & Entering UNBLOCK state
  SIGINT signal unblocked
  ^C

  $ ./a.out
  Entering BLOCK state
  SIGINT signal blocked
  ^CLeaving Blocking State & Entering UNBLOCK state

*/
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc,  char *argv[])
{
	int i;
	sigset_t intmask;
	int repeatfactor;
	double y = 0.0;

	if ((sigemptyset(&intmask) == -1)
	    || (sigaddset(&intmask, SIGINT) == -1)) {
		perror("Failed to initialize the signal mask");
		return 1;
	}

	for ( ; ; ) {
		printf("Entering BLOCK state\n");
		if (sigprocmask(SIG_BLOCK, &intmask, NULL) == -1)
			break;
		fprintf(stderr, "SIGINT signal blocked\n");
		sleep(2);

		printf("Leaving Blocking State & Entering UNBLOCK state\n");
		if (sigprocmask(SIG_UNBLOCK, &intmask, NULL) == -1)
			break;
		fprintf(stderr, "SIGINT signal unblocked\n");
		sleep(2);
	}
	perror("Failed to change signal mask");
	return 1;
}
