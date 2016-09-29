[Original Copy](http://www.linuxprogrammingblog.com/all-about-linux-signals)

# Content

There have compare [select/poll/epoll](compare.md)

## sample
  - [Basic Select](select.c),
  - [Select used in server wait-all-client](select-mult-cli.c)

But we should know that select:
  - have [select signal race condition](select_issue.c) issue
  - which can be solved by [pselect](pselect.c).

# Select

## Race conditions with select

```C
// Fragment of example: Signal race with select() and accept()
while (!exit_request) {
		fd_set fds;
		int res;
 
		/* BANG! we can get SIGTERM at this point. */
 
		FD_ZERO (&fds);
		FD_SET (lfd, &fds);
 
		res = select (lfd + 1, &fds, NULL, NULL, NULL);
 
		/* accept connection if listening socket lfd is ready */
}
```
We want to be able to cleanly shut it down with a signal like SIGTERM (remove the PID file, wait for pending connections to finish etc.).
To do this we have a handler for the signal defined which sets global flag say `exit_request` here.
There is a specific case in which the signal will not interrupt the program util select timeout:
  - When it arrives between checking the while condition and executing select(2).
  - The select(2) function have no chance to be interrupted:
    + the signal was already delivered and handled before select
    + and the program will sleep at select until timeout or some file descriptor it monitors will be ready.
    + as result, the program have no reponse even user send SIGTERM util sleep timeout.

## Preventing race conditions

If we can block that signal util sleep by select(2):
  - block that signal,
  - call pselect() to wait event, at the sametime unblock that signal, if have that signal when sleep:
    + the signal handler called first,
    + then pselect() interrupt by that signal, check the global-flag and exit gracefull.
  - If read event happened, pselect() wakeup, at the sametime block that signal and process the new arrived data.

This is where the sigprocmask(2) and other "new" functions are useful. Let's see an improved version:

```C
//Fragment of example: Using pselect() to avoid a signal race
sigemptyset (&mask);
sigaddset (&mask, SIGTERM);
 
if (sigprocmask(SIG_BLOCK, &mask, &orig_mask) < 0) {
	perror ("sigprocmask");
	return 1;
}
 
while (!exit_request) {
 
	/* BANG! we can get SIGTERM at this point, but it will be
	 * delivered while we are in pselect(), because now
	 * we block SIGTERM.
	 */
 
	FD_ZERO (&fds);
	FD_SET (lfd, &fds);
 
	res = pselect (lfd + 1, &fds, NULL, NULL, NULL, &orig_mask);
 
	/* accept connection if listening socket lfd is ready */
}
```

What's the difference between select(2) and pselect(2)?

The most important one is that the later takes an additional argument of type sigset_t with set of signals that are unblocked during the execution of the system call.
The idea is that the signals are blocked, then global variables/flags that are changed in signal handlers are read and then pselect(2) runs.
There is no race because pselect(2) unblocks the signals atomically.
See the example:
  - the exit_request flag is checked while the signal is blocked, so there is no race here that would lead to executing pselect(2) just after the signal arrives.
  - In fact, in this example we block the signal all the time and the only place where it can be delivered to the program is the pselect(2) execution.
  - In real world you may block the signals only for the part of the program that contains the flag check and the pselect(2) call to allow interruption in other places in the program.

## How to block signals

Signal blocking with sigprocmask().

If we've block the signal with sigprocmask(2). The signal is not ignored, it's blocked, it means that are queued by the kernel and delivered when we unblock the signal. This is different than ignoring the signal with signal(2):
   - First sigprocmask(2) is more complicated, it operates in a set of signals represented by sigset_t, not on one signal.
	  + The SIG_BLOCK parameter tells that the the signals in set are to be blocked (in addition to the already blocked signals).
	  + The SIG_SETMASK tells that the signals in set are to be blocked, and signals that are not present in the set are to be unblocked.
	  + The third parameter, if not NULL, is written with the current signal mask. This allows to restore the mask after modifying the process' signal mask. 

## How about poll

If you like poll(2) there is analogous ppoll(2) functions, but in contrast of pselect(2) ppoll(2) is not a standard POSIX function.
