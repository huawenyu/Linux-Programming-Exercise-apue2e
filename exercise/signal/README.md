<!-- START doctoc generated TOC please keep comment here to allow auto update -->
<!-- DON'T EDIT THIS SECTION, INSTEAD RE-RUN doctoc TO UPDATE -->
**Table of Contents**  *generated with [DocToc](http://doctoc.herokuapp.com/)*

- [signal discuss](#signal-discuss)
  - [Clarification on SIGKILL, SIGTERM, SIGINT, SIGQUIT and SIGSTP](#clarification-on-sigkill-sigterm-sigint-sigquit-and-sigstp)
  - [signal handle questions](#signal-handle-questions)
    - [I'm wondering a few things:](#im-wondering-a-few-things)
    - [Answer](#answer)
      - [1. Is any signal handling necessary?](#1-is-any-signal-handling-necessary)
      - [2. Are there any other signals that I need to be concerned with regarding clean termination?](#2-are-there-any-other-signals-that-i-need-to-be-concerned-with-regarding-clean-termination)
      - [3. Does the terminate variable in my example have to be volatile?](#3-does-the-terminate-variable-in-my-example-have-to-be-volatile)
      - [4. I've read that signal() is now deprecated, and to use sigaction()](#4-ive-read-that-signal-is-now-deprecated-and-to-use-sigaction)
      - [5. Is the second call to signal() necessary? Is there something similar that I need to be concerned with for sigaction()?](#5-is-the-second-call-to-signal-necessary-is-there-something-similar-that-i-need-to-be-concerned-with-for-sigaction)
      - [6. How about just let OS do the left things?](#6-how-about-just-let-os-do-the-left-things)
- [Linux signals](#linux-signals)
  - [What is signaled in Linux](#what-is-signaled-in-linux)
  - [Signal handlers](#signal-handlers)
  - [Example use of sigaction()](#example-use-of-sigaction)
  - [Atomic Type](#atomic-type)

<!-- END doctoc generated TOC please keep comment here to allow auto update -->

# signal discuss


## Check a process react to signals

  The "$$" representing the current process.

    $ cat /proc/$$/status | grep Sig
      SigQ:   0/15432
      SigPnd: 0000000000000000
      SigBlk: 0000000000010000
      SigIgn: 0000000000380004
      SigCgt: 000000004b817efb

  First, let’s look at the definitions of each of these fields.

    SigQ   – Two slash-separated numbers that relate to queued signals for the real user ID of this process
    SigPnd – Number of pending signals for the thread and the process as a whole
    SigBlk – Signals being blocked
    SigIgn – Signals being ignored
    SigCgt – Signals being caught

  Next, let’s dig a little deeper. Looking at just the signals caught:

    $ echo 4b817efb | xxd -r -p | xxd -b
      00000000: 01001011 10000001 01111110 11111011

      01001011 10000001 01111110 11111011
       |  | || |      |  ||||||  ||||| ||
       |  | || |      |  ||||||  ||||| |+--  1 SIGHUP (hangup)
       |  | || |      |  ||||||  ||||| +---  2 SIGINT (interrupt)
       |  | || |      |  ||||||  ||||+-----  4 SIGILL (illegal instruction)
       |  | || |      |  ||||||  |||+------  5 SIGTRAP (trace/trap)
       |  | || |      |  ||||||  ||+-------  6 SIGABRT (abort)
       |  | || |      |  ||||||  |+--------  7 SIGEMT (emulation trap)
       |  | || |      |  ||||||  +---------  8 SIGFPE (floating point exception)
       |  | || |      |  |||||+------------ 10 SIGBUS (bus error)
       |  | || |      |  ||||+------------- 11 SIGSEGV (segmentation violation)
       |  | || |      |  |||+-------------- 12 SIGSYS (bad system call)
       |  | || |      |  ||+--------------- 13 SIGPIPE (broken pipe)
       |  | || |      |  |+---------------- 14 SIGALRM (alarm)
       |  | || |      |  +----------------- 15 SIGTERM (termination)
       |  | || |      +-------------------- 17 SIGUSR2 (user signal 2)
       |  | || +--------------------------- 24 SIGTSTP (stop -- can be ignored)
       |  | |+----------------------------- 25 SIGCONT (continue)
       |  | +------------------------------ 26 SIGTTIN (terminal input)
       |  +-------------------------------- 28 SIGVTALRM (timer expiration)
       +----------------------------------- 31 SIGXFSZ (file size exceeded)

  Catching a signal requires that a signal handling function exists in the process to handle a given signal.
  The SIGKILL (9) and SIGSTOP (#) signals cannot be ignored or caught.

  For example, , you would include something like this in your source code:

      signal(SIGINT, SIG_IGN); // if you wanted to tell the kernel that ctrl-C's are to be ignored

      signal(SIGSEGV, SIG_DFL);// To ensure that the default action for a signal is taken, you would do something like this instead:

  The SigIgn (signal ignore) settings from the example above show that only four signals
      3 (SIGQUIT) and 20-22 (SIGWINCH, SIGURG, and SIGPOLL) are set to be ignored,
      while the SigBlk (signal block) settings block only the SIGPIPE

      $ echo 380004 | xxd -r -p | xxd -b
      00000000: 00111000 00000000 00000100

      $ echo 10000 | xxd -r -p | xxd -b
      00000000: 00010000 00000000

  The same type of data for a watchdog process looks very different. Notice that it’s ignoring all signals.

      # cat /proc/11/status | grep Sig
        SigQ:   0/15432
        SigPnd: 0000000000000000
        SigBlk: 0000000000000000
        SigIgn: ffffffffffffffff
        SigCgt: 0000000000000000

## Signals list

```c
       First the signals described in the original POSIX.1-1990 standard.
       The signals SIGKILL and SIGSTOP cannot be caught, blocked, or ignored.

       Signal     Value     Action   Comment
       ──────────────────────────────────────────────────────────────────────
*      SIGHUP        1       Term    Hangup detected on controlling terminal
                                     or death of controlling process
*      SIGINT        2       Term    Interrupt from keyboard, Ctrl+C
*      SIGQUIT       3       Core    Quit from keyboard, Ctrl+\
       SIGILL        4       Core    Illegal Instruction
*      SIGABRT       6       Core    Abort signal from abort(3)
       SIGFPE        8       Core    Floating point exception
*#     SIGKILL       9       Term    Kill signal
*      SIGSEGV      11       Core    Invalid memory reference
       SIGPIPE      13       Term    Broken pipe: write to pipe with no
                                     readers
*      SIGALRM      14       Term    Timer signal from alarm(2)
*      SIGTERM      15       Term    Termination signal
*      SIGUSR1   30,10,16    Term    User-defined signal 1
*      SIGUSR2   31,12,17    Term    User-defined signal 2
       SIGCHLD   20,17,18    Ign     Child stopped or terminated
       SIGCONT   19,18,25    Cont    Continue if stopped
*#     SIGSTOP   17,19,23    Stop    Stop process, Ctrl+Z
       SIGTSTP   18,20,24    Stop    Stop typed at terminal
       SIGTTIN   21,21,26    Stop    Terminal input for background process
       SIGTTOU   22,22,27    Stop    Terminal output for background process



       Next the signals not in the POSIX.1-1990 standard but described in SUSv2 and POSIX.1-2001.

       Signal       Value     Action   Comment
       ────────────────────────────────────────────────────────────────────
       SIGBUS      10,7,10     Core    Bus error (bad memory access)
       SIGPOLL                 Term    Pollable event (Sys V).
                                       Synonym for SIGIO
       SIGPROF     27,27,29    Term    Profiling timer expired
       SIGSYS      12,31,12    Core    Bad argument to routine (SVr4)
       SIGTRAP        5        Core    Trace/breakpoint trap
       SIGURG      16,23,21    Ign     Urgent condition on socket (4.2BSD)
       SIGVTALRM   26,26,28    Term    Virtual alarm clock (4.2BSD)
       SIGXCPU     24,24,30    Core    CPU time limit exceeded (4.2BSD)
       SIGXFSZ     25,25,31    Core    File size limit exceeded (4.2BSD)
```

Hence, in this post, I wish to delineate these terms by consolidating my findings from stackoverflow, wikipedia and other unix internals websites. Here it goes:

- SIGKILL: Terminates a process immediately. This signal cannot be handled (caught), ignored or blocked. (The "kill -9" command in linux generates the same signal).
- SIGTERM: Terminates a process immediately. However, this signal can be handled, ignored or caught in code. Also, this is used for graceful termination of a process.
- SIGINT:  Interrupts a process. (The default action is to terminate gracefully). This too, like, SIGTERM can be handled, ignored or caught. The difference between SIGINT and SIGTERM is that the former can be sent from a terminal as input characters. This is the signal generated when a user presses Ctrl+C. (Sidenote: Ctrl+C denotes EOT(End of Transmission) for (say) a network stream)
- SIGQUIT: Terminates a process. This is different from both SIGKILL and SIGTERM in the sense that it generates a core dump of the process and also cleans up resources held up by a process. Like SIGINT, this can also be sent from the terminal as input characters. It can be handled, ignored or caught in code This is the signal generated when a user presses Ctrl+\.
- SIGSTP:  Suspends a process. This too, can be handled, ignored or blocked. Since it does not terminate the process, the process can be resumed by sending a SIGCONT signal. This signal can be generated by pressing Ctrl+Z. (Sidenote: Ctrl+Z stands for substitute character which indicates End-of-File in DOS)

## signal handle questions

[simple-linux-signal-handling](http://stackoverflow.com/questions/17942034/simple-linux-signal-handling)

I have a program that creates many threads and runs until either power is shutdown to the embedded computer, or the user uses kill or ctrlc to terminate the process.

Here's some code and how the main() looks.

```c
static int terminate = 0;  // does this need to be volatile?
static void sighandler(int signum) { terminate = 1; }

int main() {
  signal(SIGINT, sighandler);
  // ...
  // create objects, spawn threads + allocate dynamic memory
  // ...
  while (!terminate) sleep(2);
  // ...
  // clean up memory, close threads, etc.
  // ...
  signal(SIGINT, SIG_DFL);  // is this necessary?
}
```

### I'm wondering a few things:
  - Q1, Is any signal handling necessary? Apparently the OS will handle cleanup for me. Therefore, can I just replace the signal handler with just an infinite loop and let the OS gracefully exit the threads, de-allocate the memory, etc?
  - Q2, Are there any other signals that I need to be concerned with regarding clean termination? This thread "How does SIGINT relate to the other termination signals?", was useful to list all the signals I may be concerned with, but how many actually required handling?
  - Q3, Does the terminate variable in my example have to be volatile? I've seen many examples where this variable is volatile, and others where it is not.
  - Q4, I've read that signal() is now deprecated, and to use sigaction(). Are there any really good examples to show how to convert from the previous signal() call? I'm having trouble with the new structure that I have to create/pass and how it all fits together.
  - Q5, Is the second call to signal() necessary? Is there something similar that I need to be concerned with for sigaction()?
  - Q6, To be clear, all I'm trying to accomplish to to have my: main loop run until either ctrl-c or power is disconnected or something really bad happens.

### Answer

#### 1. Is any signal handling necessary?

In the particular case of the link you posted, yes. The runs of softwares that concerns network need particular operations, as warning client of the close of a socket by example, in order to not disturb its run.
In your particular case, you don't need to handle any signal for the process to be clear gracefully : your OS will do it for you.

[Q-1a] Is any signal handling necessary?

Yes, Linux will do cleanup for you. For example if you don't close a file or a socket, Linux will do the cleanup after the process terminates. But Linux may not necessary perform the clean up immediately and it may take some time (may be to keep system performance high or some other issues). For example if you don't close a tcp-socket and the program terminates the kernel will not close the socket immediately to ensure all data has been transmitted, TCP guarantees delivery if possible.

[Q-1b] Therefore, can I just replace the signal handler with just an infinite loop and let the OS gracefully exit the threads, de-allocate the memory, etc?

No, operating system performs do clean-up only after program terminates. While a process executes, resources that are allocated to that process don't get claimed by the OS. (The OS can't know whether your process is in an infinite loop or not - this is an unsolvable problem). If you want that after process termination the OS performs the clean-up operations for you, then you don't need to handle signals (even in case your process abnormally terminated by a signal).

#### 2. Are there any other signals that I need to be concerned with regarding clean termination?

First, take a look at his page : [The GNU Library Signals](http://www.cs.utah.edu/dept/old/texinfo/glibc-manual-0.02/library_21.html#SEC337) The termination signals is what you look after. But take a look at SIGUSR1 and SIGUSR2, even if you 'll never find them in any software, except for debugging purposes.

All of this termination signals need to be handled if you don't want your soft to terminate all of a sudden.

#### 3. Does the terminate variable in my example have to be volatile?

The flag terminate should be volatile sig_atomic_t:

Because handler functions can be called asynchronously. That is, a handler might be called at any point in the program, unpredictably. If two signals arrive during a very short interval, one handler can run within another. And it is considered better practice to declare volatile sig_atomic_t, this type are always accessed atomically, avoid uncertainty about interrupting access to a variable. volatile tells the compiler not to optimize and put it into register.
(read: [Atomic Data Access and Signal Handling](http://www.cs.utah.edu/dept/old/texinfo/glibc-manual-0.02/library_21.html#SEC358) for detail expiation).
One more reference: [24.4.7 Atomic Data Access and Signal Handling](http://www.gnu.org/software/libc/manual/html_node/Atomic-Data-Access.html#Atomic-Data-Access). Furthermore, the C11 standard in 7.14.1.1-5 indicates that only objects of volatile sig_atomic_t can be accessed from a signal handler (accessing others has undefined behavior).

#### 4. I've read that signal() is now deprecated, and to use sigaction()

Sigaction() is POSIX while signal is a C standard.

```c
// 1. Prepare struct
struct sigaction sa;
sa.sa_handler =  sighandler;

// 2. To restart functions if interrupted by handler (as handlers called asynchronously)
sa.sa_flags = SA_RESTART;

// 3. Set zero
sigemptyset(&sa.sa_mask);

/* 3b.
 // uncomment if you wants to block
 // some signals while one is executing.
sigaddset( &sa.sa_mask, SIGINT );
*/

// 4. Register signals
sigaction( SIGINT, &sa, NULL );
```

#### 5. Is the second call to signal() necessary? Is there something similar that I need to be concerned with for sigaction()?

Why you set it to default-action before program termination is unclear to me. I think the following paragraph will give you an answer:

[Handling Signals](http://support.sas.com/documentation/onlinedoc/sasc/doc750/html/lr1/z1ling.htm)

The call to signal establishes signal handling for only one occurrence of a signal. Before the signal-handling function is called, the library resets the signal so that the default action is performed if the same signal occurs again. Resetting signal handling helps to prevent an infinite loop if, for example, an action performed in the signal handler raises the same signal again. If you want your handler to be used for a signal each time it occurs, you must call signal within the handler to reinstate it. You should be cautious in reinstating signal handling. For example, if you continually reinstate SIGINT handling, you may lose the ability to interrupt and terminate your program.

The signal() function defines the handler of the next received signal only, after which the default handler is reinstated. So it is necessary for the signal handler to call signal() if the program needs to continue handling signals using a non-default handler.

Read a discussion for further reference: [When to re-enable signal handlers](http://cboard.cprogramming.com/linux-programming/150239-when-re-enable-signal-handlers.html).

```c
  #include <signal.h>
  #include <stdio.h>
  #include <stdlib.h>

  /* This flag controls termination of the main loop. */
  volatile sig_atomic_t keep_going = 1;

  /* The signal handler just clears the flag and re-enables itself. */
  void
  catch_alarm (int sig)
  {
    keep_going = 0;
    signal (sig, catch_alarm);
  }

  void
  do_stuff (void)
  {
    puts ("Doing stuff while waiting for alarm....");
  }

  int
  main (void)
  {
    /* Establish a handler for SIGALRM signals. */
    signal (SIGALRM, catch_alarm);

    /* Set an alarm to go off in a little while. */
    alarm (2);

    /* Check the flag once in a while to see when to quit. */
    while (keep_going)
      do_stuff ();

    return EXIT_SUCCESS;
  }
```
- PS: a user in IRC told me "It's necessary on some platforms (implementation-defined when) with signal(). But sigaction() should avoid that issue." Is this true?
  + The "user in IRC" is mistaken.
  + The signal() function defines the handling of the next received signal only, after which the default handling is reinstated. So it is necessary for the signal handler to call signal() if the program needs to continue handling signals using a non-default handler.
  + signal() is specified in the C standard. sigaction() is not (albeit, it is considered a better approach by some). So, yes, sigaction() will avoid the need for calling signal(). sigaction() also handles situations where a second signal comes in while handling a signal. The potential catch is that sigaction() is also not available on all target systems.
  + There are some signals that cannot be handled. There are also some signals that leave the process in an unpredictable state if the signal handler does not terminate (i.e. if the handler is used to effectively ignore the signal).

#### 6. How about just let OS do the left things?

No, there is a limitation! You can't catch all signals. Some signals are not catchable e.g. SIGKILL and SIGSTOP and both are termination signals. Quoting one:

[Macro: int SIGKILL](http://www.gnu.org/software/libc/manual/html_node/Termination-Signals.html)

The SIGKILL signal is used to cause immediate program termination. It cannot be handled or ignored, and is therefore always fatal. It is also not possible to block this signal.

So you can't make [a program that cannot be interrupted (an uninterrupted program)](http://stackoverflow.com/questions/12437648/uninterruptable-process-in-windowsor-linux)!

```c
  #include<stdio.h>
  #include<signal.h>
  #include<unistd.h>

  void sig_handler(int signo)
  {
      if (signo == SIGUSR1)
          printf("received SIGUSR1\n");
      else if (signo == SIGKILL)
          printf("received SIGKILL\n");
      else if (signo == SIGSTOP)
          printf("received SIGSTOP\n");
  }

  int main(void)
  {
      if (signal(SIGUSR1, sig_handler) == SIG_ERR)
          printf("\ncan't catch SIGUSR1\n");
      if (signal(SIGKILL, sig_handler) == SIG_ERR)
          printf("\ncan't catch SIGKILL\n");
      if (signal(SIGSTOP, sig_handler) == SIG_ERR)
          printf("\ncan't catch SIGSTOP\n");
      // A long long wait so that we can easily issue a signal to this process
      while(1)
          sleep(1);
      return 0;
  }

  OUTPUT:
  $ kill -USR1 2678
  $ ./sigfunc
  can't catch SIGKILL
  can't catch SIGSTOP
  received SIGUSR1
```

# Linux signals

http://www.linuxprogrammingblog.com/all-about-linux-signals?page=show

## Send signals

On Unix systems, there are several ways to send signals:
  - to processes—with a kill command,
  - with a keyboard sequence (like control-C),
  - or through your own program (e.g., using a kill command in C).
  - Signals are also generated by hardware exceptions such as segmentation faults and illegal instructions,
  - timers and child process termination.

## What is signaled in Linux

Your process may receive a signal when:
  - From user space from some other process when someone calls a function like kill(2).
  - When you send the signal from the process itself using a function like abort(3).
  - When a child process exits the operating system sends the SIGCHLD signal.
  - When the parent process dies or hangup is detected on the controlling terminal SIGHUP is sent.
  - When user interrupts program from the keyboard SIGINT is sent.
  - When the program behaves incorrectly one of SIGILL, SIGFPE, SIGSEGV is delivered.
  - When a program accesses memory that is mapped using mmap(2) but is not available
      (for example when the file was truncated by another process) - really nasty situation when using mmap() to access files.
      There is no good way to handle this case.
  - When a profiler like gprof is used the program occasionally receives SIGPROF. This is sometimes problematic when you forgot to handle interrupting system functions like read(2) properly (errno == EINTR).
  - When you use the write(2) or similar data sending functions and there is nobody to receive your data SIGPIPE is delivered.
      This is a very common case and you must remember that those functions may not only exit with error and setting the errno variable but also cause the SIGPIPE to be delivered to the program.
      An example is the case when you write to the standard output and the user uses the pipeline sequence to redirect your output to another program.
      If the program exits while you are trying to send data SIGPIPE is sent to your process.
      A signal is used in addition to the normal function return with error
        because this event is asynchronous and you can't actually tell how much data has been successfully sent.
      This can also happen when you are sending data to a socket.
      This is because data are buffered and/or send over a wire so are not delivered to the target immediately
        and the OS can realize that can't be delivered after the sending function exits.
  - For a complete list of signals see the signal(7) manual page.

## Signal handlers

What is the difference between signal(2) and sigaction(2) if you don't use any additional feature the later one provides?
The answer is: `portability and no race conditions`:
  - The issue with resetting the signal handler after it's called doesn't affect sigaction(2),
      because the default behavior is not to reset the handler and blocking the signal during it's execution.
      So there is no race and this behavior is documented in the POSIX specification.
  - Another difference is that with signal(2) some system calls are automatically restarted and with sigaction(2) they're not by default.

Use `sigaction()` unless you've got very compelling reasons not to do so.

The `signal()` interface has antiquity (and hence availability) in its favour, and it is defined in the C standard.  Nevertheless, it has a number of undesirable characteristics that `sigaction()` avoids - unless you use the flags explicitly added to `sigaction()` to allow it to faithfully simulate the old `signal()` behaviour.

1. The `signal()` function does not (necessarily) block other signals from arriving while the current handler is executing;
   `sigaction()` can block other signals until the current handler returns.
2. The `signal()` function (usually) resets the signal action back to `SIG_DFL` (default) for almost all signals.
   This means that the `signal()` handler must reinstall itself as its first action.
   It also opens up a window of vulnerability between the time when the signal is detected and the handler is reinstalled
   during which if a second instance of the signal arrives,
   the default behaviour (usually terminate, sometimes with prejudice - aka core dump) occurs.
3. The exact behaviour of `signal()` varies between systems — and the standards permit those variations.

Whichever of the two you use, do not be tempted by the alternative signal interfaces such as
`sighold()`, `sigignore()`, `sigpause()`, `sigrelse()`
They are nominally alternatives to `sigaction()`,
but they are only barely standardized and are present in POSIX for backwards compatibility rather than for serious use.
Note that the POSIX standard says their behaviour in multi-threaded programs is undefined.

Multi-threaded programs and signals is a whole other complicated story.  <s>AFAIK, both `signal()` and `sigaction()` are OK in multi-threaded applications.</s>

> Thus, I think `sigaction()` is the only that can be used safely in a multi-threaded process.

> If the process is multi-threaded,  or if the process is single-threaded and a signal handler is executed other than as the result of:

> * The process calling `abort()`, `raise()`, `kill()`, `pthread_kill()`, or `sigqueue()` to generate a signal that is not blocked
> * A pending signal being unblocked and being delivered before the call that unblocked it returns

> the behavior is undefined if the signal handler refers to any object other than `errno` with static storage duration other than by assigning a value to an object declared as `volatile sig_atomic_t`, or if the signal handler calls any function defined in this standard other than one of the functions listed in [Signal Concepts];

Nevertheless, `sigaction()` is to be preferred in essentially all circumstances — and portable multi-threaded code should use `sigaction()` unless there's an overwhelming reason why it can't (such as "only use functions defined by Standard C" — and yes, C11 code can be multi-threaded).  Which is basically what the opening paragraph of this answer also says.

## Traditional signal() is deprecated

The signal(2) function is the oldest and simplest way to install a signal handler but it's deprecated.
There are few reasons to deprecated this old way:
  - And most important is that the original Unix implementation would reset the signal handler to it's default value after signal is received.
      If you need to handle every signal delivered to your program separately like handling SIGCHLD to catch a dying process there is a race here.
      To do so you would need to set to signal handler again in the signal handler itself and another signal may arrive before you cal the signal(2) function.
      This behavior varies across different systems.
  - Moreover, it lacks features present in sigaction(2) you will sometimes need.

```c
  #include "stdio.h"
  #include "stdlib.h"
  #include <signal.h>

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
      printf("\nchild signal handler\n");
      // The old style should install the handler again.
      signal(SIGUSR1, childHandler);
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
          alarm(5);
          while(1) {
              sleep(1);
              kill(val, SIGUSR1);
          }
          printf("\nparent exit\n");
      } else {
          signal(SIGUSR1, childHandler);
          printf("\nchild\n");
          while (1)
              pause(); // The pause() function suspends the process until a signal is caught.
          printf("\nchild exit\n");
      }

      return 0;
  }
```

## The recommended: sigaction

http://www.linuxprogrammingblog.com/all-about-linux-signals?page=show

The sigaction(2) function is a better way to set the signal action.
As you can see you don't pass the pointer to the signal handler directly, but instead a struct sigaction object.
It's defined as:

```c
int sigaction (int signum, const struct sigaction *act, struct sigaction *oldact);

struct sigaction {
        void     (*sa_handler)(int);
        void     (*sa_sigaction)(int, siginfo_t *, void *);
        sigset_t sa_mask;
        int      sa_flags;
        void     (*sa_restorer)(void);
};
```

For a detailed description of this structure's fields see the sigaction(2) manual page. Most important fields are:
  - sa_handler   - This is the pointer to your handler function that has the same prototype as a handler for signal(2).
  - sa_sigaction - This is an alternative way to run the signal handler.
                     It has two additional arguments beside the signal number where the siginfo_t * is the more interesting.
                     It provides more information about the received signal, I will describe it later.
  - sa_mask      - allows you to explicitly set signals that are blocked during the execution of the handler.
                     In addition if you don't use the SA_NODEFER flag the signal which triggered will be also blocked.
  - sa_flags     - allow to modify the behavior of the signal handling process.
                     To use the sa_sigaction handler you must use SA_SIGINFO flag here.

## Example sigaction(): volatile

In this example we use the three arguments version of signal handler for SIGTERM.
Without setting the SA_SIGINFO flag we would use a traditional one argument version of the handler
  and pass the pointer to it by the sa_handler field.
It would be a replacement for signal(2). You can try to run it and do kill PID to see what happens.

```c
  /* Example of using sigaction() to setup a signal handler with 3 arguments
   * including siginfo_t.
   */
  #include <stdio.h>
  #include <unistd.h>
  #include <signal.h>
  #include <string.h>

  // In the signal handler we read two fields from the `siginfo_t *siginfo` parameter to read the sender's PID and UID.
  static void hdl (int sig, siginfo_t *siginfo, void *context)
  {
  	printf ("Sending PID: %ld, UID: %ld\n",
  			(long)siginfo->si_pid, (long)siginfo->si_uid);
  }

  int main (int argc, char *argv[])
  {
  	struct sigaction act;

  	memset (&act, '\0', sizeof(act));
  	/* Use the sa_sigaction field because the handles has two additional parameters */
  	act.sa_sigaction = &hdl;
  	/* The SA_SIGINFO flag tells sigaction() to use the sa_sigaction field, not sa_handler. */
  	act.sa_flags = SA_SIGINFO;

  	if (sigaction(SIGTERM, &act, NULL) < 0) {
  		perror ("sigaction");
  		return 1;
  	}

  	while (1)
  		sleep (10); // used in a loop because it's interrupted when the signal arrives and must be called again.

  	return 0;
  }
```

SA_SIGINFO handler

In the previous example SA_SIGINFO is used to pass more information to the signal handler as arguments.

We've seen that the siginfo_t structure contains si_pid and si_uid fields (PID and UID of the process that sends the signal), but there are many more.
  - On Linux only si_signo (signal number) and si_code (signal code) are available for all signals.
  - Presence of other fields depends on the signal type. Some other fields are:
      + si_code - Reason why the signal was sent. It may be
        - SI_USER if it was delivered due to kill(2) or raise(3),
        - SI_KERNEL if kernel sent it and few more.
        - For some signals there are special values like ILL_ILLADR telling you that SIGILL was sent due to illegal addressing mode.
      + For SIGCHLD fields si_status, si_utime, si_stime are filled and contain information about the exit status or the signal of the dying process, user and system time consumed.
      + In case of SIGILL, SIGFPE, SIGSEGV, SIGBUS `si_addr` contains the memory address that caused the fault.

Compiler optimization and data in signal handler

Let's see the following example:
```c
  #include <stdio.h>
  #include <unistd.h>
  #include <signal.h>
  #include <string.h>

  // @note should add volatile keyword
  //static int exit_flag = 0;
  static volatile int exit_flag = 0;

  static void hdl (int sig)
  {
  	exit_flag = 1;
  }

  int main (int argc, char *argv[])
  {
  	struct sigaction act;

  	memset (&act, '\0', sizeof(act));
  	act.sa_handler = &hdl;
  	if (sigaction(SIGTERM, &act, NULL) < 0) {
  		perror ("sigaction");
  		return 1;
  	}

  	while (!exit_flag)
  		;

  	return 0;
  }
```

What it does? It depends on compiler optimization settings.
  - Without optimization it executes a loop that ends when the process receives SIGTERM or other sgnal that terminates the process and was not handler.
  - When you compile it with the -O3 gcc flag it will not exit even after receiving SIGTERM.
      Why?  because the while loop is optimized in such way that the exit_flag variable is loaded into a processor register once and not read from the memory in the loop.
      The compiler isn't aware that the loop is not the only place where the program accesses this variable while running the loop.
      In such cases - modifying a variable in a signal handler that is also accessed in some other parts of the program
      you must remember to instruct the compiler to always access this variable in memory when reading or writing them.
      You should use the `volatile` keyword in the variable declaration:

      static volatile int exit_flag = 0;

    After this change everything works as expected.

## Atomic type: sig_atomic_t

There is one data type defined that is guaranteed to be atomically read and written both in signal handlers and code that uses it: sig_atomic_t.
The size of this type is undefined, but it's an integer type.
In theory this is the only type you can safely assign and read if it's also accessed in signal handlers.
Keep in mind that:
It doesn't work like a mutex: it's guaranteed that read or write of this type translates into an uninterruptible operation but code such as:

```c
sig_atomic_t i = 0;
void sig_handler (int sig)
{
	if (i++ == 5) {
		// ...
	}
}
```
Isn't safe: there is read and update in the if operation but only single reads and single writes are atomic.

Don't try to use this type in a multi-threaded program as a type that can be used without a mutex. It's only intended for signal handlers and has nothing to do with mutexes!
You don't need to worry if data are modified or read in a signal handler are also modified or read in the program if it happens only in parts where the signal is blocked.
Later I'll show how to block signals. But you will still need the volatile keyword.


## Signal-safe functions

You can not just do anything in a signal handler.
Remember that your program is interrupted, you don't know at which point, which data objects are in the middle of being modified.
It may be not only your code, but a library you are using or the standard C library.
In fact there is a quite short list of function you can safely call from a signal handler in signal(7).
You can for example open a file with `open(2)`, remove a file with `unlink(2)`, call `_exit(2)` (but not `exit(3)!`) and more.
In practice this list is so limited that the best you can do is to just set some global flag to notify the process to do something like exiting.
On the other hand the `wait(2)` and `waitpid(2)` functions can be used,
so you can cleanup dead processes in SIGCHLD, `unlink(2)` is available, so you can delete a pid file etc.

## Alternative method of handling signals: signalfd()

signalfd(2) is a quite new Linux specific call available from the 2.6.22 kernel that allows to receive signals using a file descriptor.
This allows to handle signals in a synchronous way, without providing handler functions. Let's see an example of signalfd() use
First we must block the signals we want to handle with signalfd(2) using sigprocmask(2).
This function will be described later.
Then we call signalfd(2) to create a file descriptor that will be used to read incoming signals.
At this point in case of SIGTERM or SIGINT delivered to your program it will not be interrupted, no handler will be called.
It will be queued and you can read information about it from the sfd descriptor.
You must supply a buffer large enough to read the struct signalfd_siginfo object that will be filled with information similar to the previously described siginfo_t.
The difference is that the fields are named a bit different (like ssi_signo instead of si_signo).

What is interesting is that the sfd descriptor behaves and can be used just like any other file descriptor, in particular you can:
- Use it in select(2), poll(2) and similar functions.
- Make it non-blocking.
- Create many of them, each handling different signals to return different descriptors as ready by select(2) for every different signal.
- After fork() the file descriptor is not closed, so the child process can read signals that were send to the parent.

This is perfect to be used in a single-process server with the main loop executes a function like poll(2) to handle many connections.
It simplifies signal handling because the signal descriptor can be added to the poll's array of descriptors and handled like any other of them, without asynchronous actions.
You handle the signal when you are ready for that because your program is not interrupted.

```c
  /* Example of use of a Linux-specific call - signalfd() to handle signals using
   * a file descriptor.
   */
  #include <stdio.h>
  #include <unistd.h>
  #include <signal.h>
  #include <sys/signalfd.h>
  #include <string.h>

  int main (int argc, char *argv[])
  {
  	int sfd;
  	sigset_t mask;

  	/* We will handle SIGTERM and SIGINT. */
  	sigemptyset (&mask);
  	sigaddset (&mask, SIGTERM);
  	sigaddset (&mask, SIGINT);

  	/* Block the signals that we handle using signalfd(), so they don't
  	 * cause signal handlers or default signal actions to execute. */
  	if (sigprocmask(SIG_BLOCK, &mask, NULL) < 0) {
  		perror ("sigprocmask");
  		return 1;
  	}

  	/* Create a file descriptor from which we will read the signals. */
  	sfd = signalfd (-1, &mask, 0);
  	if (sfd < 0) {
  		perror ("signalfd");
  		return 1;
  	}

  	while (1) {

  		/** The buffor for read(), this structure contains information
  		 * about the signal we've read. */
  		struct signalfd_siginfo si;

  		ssize_t res;

  		res = read (sfd, &si, sizeof(si));
  		if (res < 0) {
  			perror ("read");
  			return 1;
  		}
  		if (res != sizeof(si)) {
  			fprintf (stderr, "Something wrong\n");
  			return 1;
  		}

  		if (si.ssi_signo == SIGTERM)
  			printf ("Got SIGTERM\n");
  		else if (si.ssi_signo == SIGINT) {
  			printf ("Bye!\n");
  			break;
  		}
  		else {
  			fprintf (stderr, "Got some unhandled signal\n");
  			return 1;
  		}
  	}

  	/* Close the file descriptor if we no longer need it. */
  	close (sfd);

  	return 0;
  }
```

# Example: SIGALRM

```c
  /*
  output:
      In child process... 1
      In child process... 2
      In child process... 3
      In child process... 4
      In child process... 5
      Caught an SIGALRM signal.
      Signal value = 14
      Exiting from process...

      parent process wait=0... 0
      In parent process... 1
      In parent process... 2
      In parent process... 3
      In parent process... 4
      In parent process... 5
      In parent process... 6
      In parent process... 7
      Caught an SIGALRM signal.
      Signal value = 14
      Exiting from process...
  */

  #include <stdio.h>
  #include <stdlib.h>
  #include <unistd.h>
  #include <signal.h>
  #include <wait.h>
  #include <sys/types.h>

  static void catchAlarm(int signo);

  int main(void)
  {
      int status;
      pid_t pid;

      signal(SIGALRM, catchAlarm);
      setbuf(stdout, NULL);
      pid = fork();

      if (pid < 0) {
          printf("Problem forking process.\n");
          printf("Exiting now...\n");
          exit(EXIT_FAILURE);
      }

      if (pid==0) {
          int i = 0;

          alarm(5);
          while (1) {
              i++;
              printf("In child process... %d\n", i);
              sleep(1);
          }
      } else {
          int i = 0;

          wait(&status);
          printf("parent process wait=%d... %d\n", i);
          alarm(7);
          while (1) {
              i++;
              printf("In parent process... %d\n", i);
              sleep(1);
          }
      }
  }

  static void catchAlarm(int signo)
  {
      printf("Caught an SIGALRM signal.\n");
      printf("Signal value = %d\n", signo);
      printf("Exiting from process...\n");
      exit(EXIT_SUCCESS);
  }
```

# Example: SIGCHLD

This is a signal sent to the parent when a child changes status.

I need to create a c file that .

```c
  #include <stdlib.h>
  #include <unistd.h>
  #include <sys/types.h>
  #include <sys/wait.h>

  int main(int argc, char *argv[])
  {
      int W;
      int T;
      int status;
      pid_t pid;

      // takes in two arguments WAIT, and TIME
      if(argc != 3)
          return 1;
      W = atoi(argv[1]);
      T = atoi(argv[2]);
      pid = fork();
      if (pid == 0) {
          sleep(W);
      }
      else {
          alarm(T);
          /* wait will block the parent until it receives a signal--either SIGCHLD or another unignored signal.
             This means your strategy of using alarm should work perfectly
             (however, you should note that the default action for SIGALRM is to terminate the program,
              so if that's not what you want, you'll need to change it):
           */
          if (wait() == -1) {
              /*
              This will not actually run unless you override
              the default action for SIGALRM
              */
          }
          else {
               /* Child changed state
                  Check how with WIF... macros
                  - The child terminates normally
                  - The child was terminated by a signal
                  - The child was stopped by a signal
                  - The child was continued by a signal
              */
          }
      }
      return 0;
  }
```

## Simplest dead child cleanup in a SIGCHLD handler

If you create new processes in your program and don't really want to wait until they exit, and possibly their exist status doesn't matter, just want to cleanup zombie processes you can create a SIGCHLD handler that does just that and forget about process you've created. This handler can look like this one:

```c
  /* Simplest dead child cleanup in a SIGCHLD handler. Prevent zombie processes
   * but don't actually do anything with the information that a child died.
   */
  #include <sys/types.h>
  #include <sys/wait.h>
  #include <signal.h>
  #include <stdio.h>
  #include <string.h>
  #include <stdlib.h>
  #include <unistd.h>
  #include <errno.h>

  /* SIGCHLD handler. */
  static void sigchld_hdl (int sig)
  {
  	/* Wait for all dead processes.
  	 * We use a non-blocking call to be sure this signal handler will not
  	 * block if a child was cleaned up in another part of the program. */
  	while (waitpid(-1, NULL, WNOHANG) > 0) {
  	}
  }

  int main (int argc, char *argv[])
  {
  	struct sigaction act;
  	int i;

  	memset (&act, 0, sizeof(act));
  	act.sa_handler = sigchld_hdl;

  	if (sigaction(SIGCHLD, &act, 0)) {
  		perror ("sigaction");
  		return 1;
  	}

  	/* Make some children. */
  	for (i = 0; i < 50; i++) {
  		switch (fork()) {
  			case -1:
  				perror ("fork");
  				return 1;
  			case 0:
  				return 0;
  		}
  	}

  	/* Wait until we get a sleep() call that is not interrupted by a signal. */
  	while (sleep(1)) {
  	}

  	return 0;
  }
```
