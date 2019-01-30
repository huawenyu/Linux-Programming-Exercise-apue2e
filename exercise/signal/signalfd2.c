#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/signalfd.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <assert.h>
#include <errno.h>
#include <netinet/in.h>
#include <stropts.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>

volatile sig_atomic_t cont = 1;
volatile sig_atomic_t usrcnt = 0;
volatile sig_atomic_t susrcnt = 0;

int main()
{
	sigset_t mask;
	struct sigaction act;
	int sfd, pid, i, rc;
	int on = 1;

	setbuf(stdout, NULL);
	printf("My PID: %d\n", getpid());
	printf("SIGSEGV: %d\nSIGFPE: %d\nSIGUSR1: %d\n", SIGSEGV, SIGFPE, SIGUSR1);

	sigemptyset(&mask);
	sigaddset(&mask, SIGUSR1);
	sigprocmask(SIG_BLOCK, &mask, NULL);

	sfd = signalfd(-1, &mask, 0);
	if (sfd < 0) {
		perror("signalfd");
		return 1;
	}
	rc = ioctl(sfd, FIONBIO, (char *)&on);
	if (rc < 0) {
		perror("ioctl() failed");
		close(sfd);
		exit(-1);
	}

	pid = fork();
	if (pid) { /* parent */
		int status;

		sleep(1); //To avoid race between signal handler and signal fd.
		for (i=0;i<3;++i) {
			union sigval value;

			//If sleep is not used, signal SIGUSR1 will be handled one time in parent
			//as other signals will be ignored while SIGUSR1 is being handled.
			sleep(1);

			//Problem is here. When the sleep(1) is commented out, it missed the signals.
			kill(pid, SIGUSR1);

			//value.sival_int = 1234;
			//sigqueue(pid, SIGUSR1, value);
		}

		puts("I'm waiting for my child to complete.\n");
		if (wait(&status) == -1) {
			/*
			   This will not actually run unless you override
			   the default action for SIGALRM
			   */
			perror("wait() error");
		}
		else if (WIFEXITED(status))
			printf("The child exited with status of %d\n",
			       WEXITSTATUS(status));
		else
			puts("The child did not exit successfully");
		puts("parent exit.");
	}
	else { /* child */
		struct pollfd fds[1];
		int ret;
		ssize_t bytes;

		memset(fds, 0 , sizeof(fds));
		fds[0].fd = sfd;
		fds[0].events = POLLIN | POLLERR | POLLHUP;

		for (;;) {
			struct signalfd_siginfo info;

			ret = poll(fds, 1, -1);

			/* Bail on errors (for simplicity) */
			assert(ret > 0);
			assert(fds[0].revents & POLLIN);

try_again:
			bytes = read(sfd, &info, sizeof(info));
			printf ("read: bytes=%d errno=%d, EINTR=%d EAGAIN=%d\n",
				bytes, errno, EINTR, EAGAIN);
			if (bytes < 0) {
				if (errno == EINTR)
					goto try_again;
				else if (errno == EAGAIN)
					continue;
			}
			assert(bytes == sizeof(info));

			unsigned sig = info.ssi_signo;
			unsigned user = info.ssi_uid;

			if (sig == SIGUSR1) {
				++usrcnt;
				printf ("Got SIGUSR1 by POLL in thread: %d: Handler count: %d,  %d\n", getpid(), susrcnt, usrcnt);
			}
			else
				printf ("Got other signal by POLL in thread: %d: Handler count: %d,  %d\n", getpid(), susrcnt, usrcnt);
			goto try_again;
		}
	}

	/* Close the file descriptor if we no longer need it. */
	close (sfd);
	pause();
	return 0;
}

