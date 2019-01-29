/**Usage: ./process_child <child-sleep> <parent-wait>

  ./process_child 5 3
  ===================
  This is the parent.
  1543440249: Parent's pid is 3225 and my child's is 3226
  I'm waiting for my child to complete.

  This is the child.
  1543440249: Child's pid is 3226 and my parent's is 3225

  [1] 3225 alarm      ./process_child 5 3      <=== parent receive alarm signal and terminate


  child exit.
  1543440254: Process 3226 atexit!


  ./process_child 3 5
  ===================
  This is the parent.
  1543440325: Parent's pid is 4067 and my child's is 4068
  I'm waiting for my child to complete.

  This is the child.
  1543440325: Child's pid is 4068 and my parent's is 4067


  child exit.
  1543440328: Process 4068 atexit!

  1543440328: The child exited with status of 42
  parent exit.
  1543440328: Process 4067 atexit!

 */
#define _POSIX_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#ifndef NDEBUG
#include <time.h>
#define printf(s, ...) printf("%lu: " s, (unsigned long)time(NULL), __VA_ARGS__)
#endif

/* Not called if signal trigger terminated */
void atexit_info(void)
{
	printf("Process %d atexit!\n", (int)getpid());
}

int main(int argc, char *argv[])
{
	int W;
	int T;
	int status;
	pid_t pid;

	atexit(atexit_info);
	if(argc != 3) {
		puts("Usage: ./this <wait-sec> <alarm-sec>\n");
		return 1;
	}

	W = atoi(argv[1]);
	T = atoi(argv[2]);
	pid = fork();
	if (pid == 0) {
		puts("This is the child.");
		printf("Child's pid is %d and my parent's is %d\n",
		       (int)getpid(), (int)getppid());

		sleep(W);
		puts("child exit.");
		exit(42);
	}
	else {
		puts("This is the parent.");
		printf("Parent's pid is %d and my child's is %d\n",
		       (int)getpid(), (int)pid);

		/*
		  The alarm() function sets a timer that generates the SIGALARM signal.
		  If we ignore or don’t catch this signal, the process is terminated.
		  */
		alarm(T);

		puts("I'm waiting for my child to complete.");
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
	return 0;
}

