/* The issue of this code:
 * - the signal sender must sleep(), otherwise the same signal will be merged into one signal
 */
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/signalfd.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <poll.h>
#include <assert.h>
#include <errno.h>


volatile sig_atomic_t cont = 1;
volatile sig_atomic_t usrcnt = 0;
volatile sig_atomic_t susrcnt = 0;

volatile sig_atomic_t wsig = 0;
volatile sig_atomic_t wtid = 0;

int GetCurrentThreadId()
{
	return syscall(__NR_gettid);
}

void Segv1(int p1, siginfo_t * p2, void * p3)
{
	//printf("SIGSEGV signal on illegal memory access handled by thread: %d\n", GetCurrentThreadId());
	wtid = GetCurrentThreadId();
	wsig = SIGSEGV;
	_exit(SIGSEGV);
}

void Fpe1(int p1 , siginfo_t * p2, void * p3)
{
	//printf is only for test.
	//printf("FPE signal handled by thread: %d\n", GetCurrentThreadId());
	wtid = GetCurrentThreadId();
	wsig = SIGFPE;
	_exit(SIGFPE);
}

void User1(int p1 , siginfo_t * p2, void * p3)
{
	printf("User signal 1 handled by thread: %d\n", GetCurrentThreadId());
	++susrcnt;
	wtid = GetCurrentThreadId();
	wsig = SIGUSR1;
}


void* ThreadFunc (void* d)
{

	//Let us use signalfd.
	int sfd;
	sigset_t mask;

	/* We will handle SIGTERM and SIGINT. */
	sigemptyset (&mask);
	sigaddset (&mask, SIGUSR1);

	/* Create a file descriptor from which we will read the signals. */
	sfd = signalfd (-1, &mask, 0);
	if (sfd < 0) {
		printf ("signalfd failed with %d\n", errno);
		return NULL;
	}

	pthread_sigmask(SIG_BLOCK, &mask, NULL);

	/* This is the main loop */
	struct pollfd fds[1];
	int ret;
	ssize_t bytes;

	memset(fds, 0 , sizeof(fds));
	fds[0].fd = sfd;
	fds[0].events = POLLIN | POLLERR | POLLHUP;

	for (;;) {
		ret = poll(fds, 1, -1);

		/* Bail on errors (for simplicity) */
		assert(ret > 0);
		assert(fds[0].revents & POLLIN);

		/* We have a valid signal, read the info from the fd */
		struct signalfd_siginfo info;
		bytes = read(sfd, &info, sizeof(info));
		assert(bytes == sizeof(info));

		unsigned sig = info.ssi_signo;
		unsigned user = info.ssi_uid;

		if (sig == SIGUSR1) {
			++usrcnt;
			printf ("Got SIGUSR1 by POLL in thread: %d: Handler count: %d,  %d\n", GetCurrentThreadId(), susrcnt, usrcnt);
		}
		else
			printf ("Got other signal by POLL in thread: %d: Handler count: %d,  %d\n", GetCurrentThreadId(), susrcnt, usrcnt);
	}

	/* Close the file descriptor if we no longer need it. */
	close (sfd);
	return NULL;
}


int main()
{
	const int numthreads = 1;
	sigset_t sset;
	struct sigaction act;
	int sleepval = 15;
	int pid;
	int i;
	int * a = 0;
	//*a = 1;
	int c=0;
	//c = 0;
	int b;

	setbuf(stdout, NULL);
	printf("My PID: %d\n", getpid());
	printf("SIGSEGV: %d\nSIGFPE: %d\nSIGUSR1: %d\n", SIGSEGV, SIGFPE, SIGUSR1);
	//Create a thread for signal
	memset(&act, 0, sizeof act);
	act.sa_sigaction = User1;
	act.sa_flags    = SA_SIGINFO;

	//Set Handler for SIGUSR1 signal.
	if (sigaction(SIGUSR1, &act, NULL)<0) {
		fprintf(stderr, "sigaction failed\n");
		return 1;
	}

	//Set handler for SIGSEGV signal.
	act.sa_sigaction    = Segv1;
	sigaction(SIGSEGV, &act, NULL);

	//Set handler for SIGFPE (floating point exception) signal.
	act.sa_sigaction    = Fpe1;
	sigaction(SIGFPE, &act, NULL);
	sigemptyset(&sset);
	sigaddset(&sset, SIGUSR1);
	sigprocmask(SIG_UNBLOCK, &sset, NULL);

	pthread_t tid[numthreads];
	for (i=0;i<numthreads;++i)
		pthread_create(&tid[i], NULL, ThreadFunc, NULL);

	//Block the signal for main thread so that other thread handles the the signal.
	pthread_sigmask(SIG_BLOCK, &sset, NULL);
	sleep(numthreads/2);

	//Raise user signal SIGUSR1.
	//raise(SIGUSR1);
	pid = fork();
	if (pid) {
		while(sleepval) {
			sleepval = sleep(sleepval);
			if(sleepval)
				switch(wsig) {
				case SIGSEGV:
					printf("[Main] Segmenation fault in thread: %d\n", wtid);
					exit(1);
					break;
				case SIGFPE:
					printf("[Main] Floating point exception in thread: %d\n", wtid);
					exit(1);
					break;
				case SIGUSR1:
					printf("[Main] User 1 signal in thread: %d\n", wtid);
					break;
				default:
					printf("[Main] Unhandled signal: %d in thread: %d\n", wsig, wtid);
					break;
				}
		}
	} else {
		sleep(1); //To avoid race between signal handler and signal fd.
		for (i=0;i<10;++i) {
			union sigval value;

			//If sleep is not used, signal SIGUSR1 will be handled one time in parent
			//as other signals will be ignored while SIGUSR1 is being handled.
			//sleep(1);

			//Problem is here. When the sleep(1) is commented out, it missed the signals.
			//kill(getppid(), SIGUSR1);

			value.sival_int = 1234;
			sigqueue(getppid(), SIGUSR1, value);
		}
		return 0;
	}

	return 0;
}

