
  #include "stdio.h"
  #include "stdlib.h"
  #include <signal.h>

  //#define MY_SIGTEST  SIGUSR1
  #define MY_SIGTEST  34

  /* For a real-world program, printing from a signal handler is not very safe.
   * A signal handler should do as little as it can, preferably only setting a flag here or there.
   * And the flag should be declared `volatile`.
   * Real-world example:
   *   I once worked on a system that used an Access database as a back end,
   *   and under certain circumstances a printf() call in a signal handler would write to the .mdb file instead of stdout,
   *   hosing the database beyond repair.
   */
  void alarmHandler(int sig)
  {
      printf("\nparent signal alarm handler: times up\n");
      exit(0);
  }

  void childHandler(int sig)
  {
      static int cnt = 0;

      printf("\nchild signal handler %d\n", cnt++);
      // The old style should install the handler again.
      signal(MY_SIGTEST, childHandler);
  }

  static void childHandler2(int sig, siginfo_t *siginfo, void *context)
  {
      static int cnt = 0;
      printf ("child signal handler %d  Sending PID: %ld, UID: %ld\n",
              cnt++, (long)siginfo->si_pid, (long)siginfo->si_uid);
  }

  int main()
  {
      pid_t val;

      signal(SIGALRM, alarmHandler);

      // If not set this, we cann't the child's output.
      // The stdout stream is buffered, so will only display what's in the buffer after it reaches a newline (or when it's told to).
      setbuf(stdout, NULL);
      if ((val = fork())) { //parinte
          printf("\nparent\n");
          // @Note that there is only one alarm clock per process.
          // the alarm() function will return a value if another alarm has been previously set.
          //
          // sets a timer that generates the SIGALARM signal when times up.
          // If we ignore or don’t catch this signal, the process is terminated.
          alarm(50);
          sleep(3);
          for (int i=0; i < 100; i++) {
              //sleep(1);
              kill(val, MY_SIGTEST);
          }
          printf("\nparent exit\n");
      } else {
          struct sigaction act;

          signal(MY_SIGTEST, childHandler);

          /*
          memset (&act, '\0', sizeof(act));
          // Use the sa_sigaction field because the handles has two additional parameters
          act.sa_sigaction = &childHandler2;
          // The SA_SIGINFO flag tells sigaction() to use the sa_sigaction field, not sa_handler.
          act.sa_flags = SA_SIGINFO;
          sigaction(MY_SIGTEST, &act, NULL);
          */

          printf("\nchild\n");
          while (1)
              pause(); // The pause() function suspends the process until a signal is caught.
          printf("\nchild exit\n");
      }

      return 0;
  }
