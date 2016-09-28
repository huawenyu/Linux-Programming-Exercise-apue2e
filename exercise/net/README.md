# Issue

[Example](select.c), but have [Race issue](select_issue.c).


How to block signals


There is sometime a need to block receiving some signals, not handling them. Traditional way is to use the deprecated signal(2) function with SIG_IGN constant as a signal handler. There is also newer, recommended function to do that: sigprocmask(2). It has a bit more complex usage, let's see an example of signal blocking with sigprocmask().
This program will sleep for 10 seconds and will ignore the SIGTERM signal during the sleep. It works this way because we've block the signal with sigprocmask(2). The signal is not ignored, it's blocked, it means that are queued by the kernel and delivered when we unblock the signal. This is different than ignoring the signal with signal(2). First sigprocmask(2) is more complicated, it operates in a set of signals represented by sigset_t, not on one signal. The SIG_BLOCK parameter tells that the the signals in set are to be blocked (in addition to the already blocked signals). The SIG_SETMASK tells that the signals in set are to be blocked, and signals that are not present in the set are to be unblocked. The third parameter, if not NULL, is written with the current signal mask. This allows to restore the mask after modifying the process' signal mask. We do it in this example. The first sleep(3) function is executed with SIGTERM blocked, if the signal arrives at this moment, it's queued. When we restore the original signal mask, we unblock SIGTERM and it's delivered, the signal handler is called.

See the sigprocmask(2) manual on how to use this function and sigsetops(3) on how to manipulate signal sets.

Preventing race conditions.


In the previous example nothing really useful was presented, such use of sigprocmask(2) isn't very interesting. Here is a bit more complex example of code that really needs sigprocmask(2):
Fragment of example: Signal race with select() and accept()
while (!exit_request) {
		fd_set fds;
		int res;
 
		/* BANG! we can get SIGTERM at this point. */
 
		FD_ZERO (&fds);
		FD_SET (lfd, &fds);
 
		res = select (lfd + 1, &fds, NULL, NULL, NULL);
 
		/* accept connection if listening socket lfd is ready */
}
Let's say it's an example of a network daemon that accepts connections using select(2) and accept(2). It can use select(2) because it listens on multiple interfaces or waits also for some events other than incoming connections. We want to be able to cleanly shut it down with a signal like SIGTERM (remove the PID file, wait for pending connections to finish etc.). To do this we have a handler for the signal defined which sets global flag and relay on the fact that select(2) will be interrupted when the signal arrives at the moment we are just waiting for some events. If the main loop in the program looks similarly as the above code everything works... almost. There is a specific case in which the signal will not interrupt the program even if it does nothing at all at the moment. When it arrives between checking the while condition and executing select(2). The select(2) function will not be interrupted (because signal was handled) and will sleep until some file descriptor it monitors will be ready.

This is where the sigprocmask(2) and other "new" functions are useful. Let's see an improved version:

Fragment of example: Using pselect() to avoid a signal race
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
What's the difference between select(2) and pselect(2)? The most important one is that the later takes an additional argument of type sigset_t with set of signals that are unblocked during the execution of the system call. The idea is that the signals are blocked, then global variables/flags that are changed in signal handlers are read and then pselect(2) runs. There is no race because pselect(2) unblocks the signals atomically. See the example: the exit_request flag is checked while the signal is blocked, so there is no race here that would lead to executing pselect(2) just after the signal arrives. In fact, in this example we block the signal all the time and the only place where it can be delivered to the program is the pselect(2) execution. In real world you may block the signals only for the part of the program that contains the flag check and the pselect(2) call to allow interruption in other places in the program.
Another difference not related to the signals is that select(2)'s timeout parameter is of type struct timeval * and pselect(2)'s is const struct timespec *. See the pselect(2) manual page for more information.

If you like poll(2) there is analogous ppoll(2) functions, but in contrast of pselect(2) ppoll(2) is not a standard POSIX function.
