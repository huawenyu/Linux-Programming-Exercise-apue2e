
# Signal repeat

```c
/*

Other comments & note:

>>> echo ---------------------
>>> echo Check signal-repeat: for i in {1..30}; do kill -34 <pid>; done
>>> {fileout}

*/

  #include <errno.h>
  #include <stdio.h>
  #include <stdlib.h>
  #include <string.h>
  #include <unistd.h>

  #include <stdarg.h>
  #include <signal.h>

  #define my_printf(...) printf(__VA_ARGS__)

  void perror2(const char *fmt, ...)
  {
      int errnum = errno;

      fprintf(stderr, "(%d: %s) ", errnum, strerror(errnum));
      va_list args;
      va_start(args, fmt);

      vfprintf(stderr, fmt, args);
      va_end(args);
      fprintf(stderr, "\n");
  }

  int sigCnt;

  void sigint_handler(int sig)
  {
     // Do thing
     sigCnt ++;
  }

  int main()
  {
      int ret;

      printf("\n  pid=%d\n", getpid());

      /* Install the SIGINT handler */
      if (signal(SIGINT, sigint_handler) == SIG_ERR)
         perror2("signal error");

      if (signal(35, sigint_handler) == SIG_ERR)
         perror2("signal error");

  again:
      ret = pause(); /* Wait for the receipt of a signal, then returns -1, and errno is set to EINTR. */
      perror2("signal delivered sigCnt=%d", sigCnt);
      if (sigCnt < 6)
          goto again;

      return 0;
  }
```


# Signal: SA_RESETHAND

```c
/*

Other comments & note:

>>> echo ---------------------
>>> echo --- TestCase1 ---
>>> echo Run without SA_RESETHAND - Still keep user-define handler after the signal handled
>>> echo Check: kill -SIGTERM `pidof vim_a.out`   <=== multiple times
>>> {fileout}
>>> echo --- TestCase2 ---
>>> echo Run with SA_RESETHAND - Just call user-define handler once, then reset to DFL (default-signal-handler)
>>> echo Check: kill -SIGTERM `pidof vim_a.out`   <=== multiple times
>>> {fileout} "with-flag-SA_RESETHAND"

*/

  #include <errno.h>
  #include <stdio.h>
  #include <stdlib.h>
  #include <string.h>
  #include <unistd.h>

  #include <stdarg.h>
  #include <signal.h>

  #define my_printf(...) printf(__VA_ARGS__)

  void perror2(const char *fmt, ...)
  {
      int errnum = errno;

      fprintf(stderr, "(%d: %s) ", errnum, strerror(errnum));
      va_list args;
      va_start(args, fmt);

      vfprintf(stderr, fmt, args);
      va_end(args);
      fprintf(stderr, "\n");
  }

  int sigcnt;
  void sigint_handler(int sig)
  {
      printf("User-define: Receive %d SIGTERM\n", ++sigcnt);
  }

  int main(int argc, char *argv[])
  {
      int ret;

      // Install handler
      struct sigaction act;

      memset(&act, 0, sizeof(act));
      act.sa_handler = sigint_handler;
      //act.sa_sigaction = sigint_handler;
      if (argc >= 2)
          act.sa_flags = SA_RESETHAND;
      sigemptyset(&act.sa_mask);

      if (0 != sigaction(SIGTERM, &act, NULL))
          perror2("signal error: sig=%d", SIGTERM);

      if (0 != sigaction(SIGTERM, &act, NULL))
          perror2("signal error: sig=%d", SIGTERM);

      int i = 0;
      for (; i < 3; i++) {
          printf("Press Enter to exit, pid=%d at %d < 3 times.\n", getpid(), i);
          getchar();
      }

      return 0;
  }
```


# Signal ignore all

```c
/*

Other comments & note:

>>> echo ========signal-ignore============
>>> echo --  cat /proc/<pid>/status| grep Sig
>>> echo --  echo fffffffe7ffbfeff | xxd -r -p | xxd -c64 -b | cut -d' ' -f6-9
>>> echo signals.sh 0 <pid>
>>> echo --------------------
>>> {fileout}

*/

  #include <errno.h>
  #include <stdio.h>
  #include <stdlib.h>
  #include <string.h>
  #include <unistd.h>

  #include <stdarg.h>
  #include <signal.h>

  void perror2(const char *fmt, ...)
  {
      int errnum = errno;

      fprintf(stderr, "(%d: %s) ", errnum, strerror(errnum));
      va_list args;
      va_start(args, fmt);

      vfprintf(stderr, fmt, args);
      va_end(args);
      fprintf(stderr, "\n");
  }

  int main()
  {
      struct sigaction act;

      act.sa_handler = SIG_IGN;
      for (int i = 1 ; i < 65 ; i++) {
          //printf("i = %d\n", i);

          // 9-SIGKILL and 19-SIGSTOP cannot be caught or ignored
          // 32 and 33 do not exist
          if (i == SIGKILL ||
              i == SIGSTOP ||
              i == 32 ||
              i == 33)
              continue;

          //if (0 != signal(i, SIG_IGN))
          if (0 != sigaction(i, &act, NULL))
              perror2("signal error: sig=%d", i);
      }

      printf("Press Enter to Continue pid=%d ", getpid());
      getchar();
      return 0;
  }
```



# Signal blocked

What's different bwteen signal-block/signal-ignore/signal-donothing-handler?

```c
/*

https://csresources.github.io/SystemProgrammingWiki/SystemProgramming/Signals,-Part-2:-Pending-Signals-and-Signal-Masks/
https://www.oracle.com/technical-resources/articles/it-infrastructure/dev-signal-handlers-studio.html
https://www.gnu.org/software/libc/manual/html_node/Why-Block.html

>>> echo ========signal-block============
>>> echo --------Check SigQ: normal-signal -----------
>>> echo --  cat /proc/<pid>/status| grep Sig
>>> echo --  kill -10 <pid>
>>> echo --  cat /proc/<pid>/status| grep Sig
>>> echo --  kill -10 <pid>
>>> echo --  cat /proc/<pid>/status| grep Sig
>>> echo --------Check SigQ: realtime-signal -----------
>>> echo --  cat /proc/<pid>/status| grep Sig
>>> echo --  kill -55 <pid>
>>> echo --  cat /proc/<pid>/status| grep Sig
>>> echo --  kill -55 <pid>
>>> echo --  cat /proc/<pid>/status| grep Sig

>>> echo signals.sh 0 <pid>
>>> echo --------------------
>>> {fileout}

*/

  #include <errno.h>
  #include <stdio.h>
  #include <stdlib.h>
  #include <string.h>
  #include <unistd.h>

  #include <stdarg.h>
  #include <signal.h>

  #define my_printf(...) printf(__VA_ARGS__)

  void perror2(const char *fmt, ...)
  {
      int errnum = errno;

      fprintf(stderr, "(%d: %s) ", errnum, strerror(errnum));
      va_list args;
      va_start(args, fmt);

      vfprintf(stderr, fmt, args);
      va_end(args);
      fprintf(stderr, "\n");
  }

  int sigIdx;
  int sigs[128];

  void sigint_handler(int sig)
  {
      sigs[sigIdx++] = sig;
  }

  int main()
  {
      int ret;
      int count = 0;
      sigset_t set, oldset;

      sigfillset(&set); // all signals
      sigprocmask(SIG_SETMASK, &set, NULL); // Block all the signals!
      // (Actually SIGKILL or SIGSTOP cannot be blocked...)

      // Install handler
      struct sigaction act;

      memset(&act, 0, sizeof(act));
      act.sa_handler = sigint_handler;
      //act.sa_sigaction = sigint_handler;
      act.sa_flags = SA_RESTART | SA_SIGINFO;
      sigemptyset(&act.sa_mask);
      for (int i=1; i < 65; i++) {
          //printf("i = %d\n", i);

          // 9-SIGKILL and 19-SIGSTOP cannot be caught or ignored
          // 32 and 33 do not exist
          if (i == SIGKILL ||
              i == SIGSTOP ||
              i == 32 ||
              i == 33)
              continue;

          if (0 != sigaction(i, &act, NULL))
              perror2("signal error: sig=%d", i);
      }

  again:
      printf("Press Enter to Un-block all pid=%d at (%d < 3) times.\n", getpid(), count);
      // for (int i=0; i < 30; i++) { sleep(1); }

      /**
      // Block and wait until a signal arrives
      while (1) {
          sigsuspend(&sig_a.sa_mask);
          printf("loop\n");
      }
      */

      getchar();

      count ++;
      if (count < 3) {
          sigemptyset(&set); // no signals
          sigprocmask(SIG_SETMASK, &set, NULL); // Un-block all the signals: set the mask to be empty
          goto again;
      }

      // Dump all triggered signals
      printf("\nSignals:");
      for (int i=0; i < 128; i++) {
          if (sigs[i])
              printf("%d ", sigs[i]);
      }
      printf("End\n");
      return 0;
  }
```


```c
/**
  Doc: https://stackoverflow.com/questions/35976496/sigactions-signal-handler-not-called-in-child-process
  KnowIssues:
  - don't know why the process repeat receive the signal and can't exit
*/

  #include <stdio.h>
  #include <stdlib.h>
  #include <signal.h>
  #include <unistd.h>

  const char * app = 0;

  void sig_handler(int signo)
  {
      printf("sig_handler(pid=%d), sleep 10.\n", getpid());

      //const pid_t p = fork();
      //if (p == 0)
      //{
      //    printf("Running app %s\n", app);
      //    execl(app, 0);
      //}

      //exit(1);

      sleep(10);
      printf("sig_handler(pid=%d) end\n", getpid());
  }


  int main(int argc, char** argv)
  {
      app = argv[0];

      struct sigaction act;
      sigemptyset(&act.sa_mask);

      act.sa_handler = sig_handler;
      act.sa_flags = 0;

      const int status = sigaction(SIGSEGV, &act, 0) == 0;
      printf("signaction = %d, pid=%d and sleep 30.\n", status, getpid());

      sleep(30);
      printf("Parent sleep over!\n");

      int* a = 0;
      int b = *a;

      return 0;
  }
```