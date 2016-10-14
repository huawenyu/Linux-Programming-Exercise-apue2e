## The child not inherit from parent process
fork() creates a new process by duplicating the calling process. The new process,
referred to as the child, is an exact duplicate of the calling process, referred to as
the parent, except for the following points:
  + The child has its own unique process ID, and this PID does not match the ID of any
    existing process group (setpgid(2)).
  + The child's parent process ID is the same as the parent's process ID.
  + The child does not inherit its parent's memory locks (mlock(2), mlockall(2)).
  + Process resource utilizations (getrusage(2)) and CPU time counters (times(2)) are
    reset to zero in the child.
  + The child's set of pending signals is initially empty (sigpending(2)).
  + The child does not inherit semaphore adjustments from its parent (semop(2)).
  + The child does not inherit record locks from its parent (fcntl(2)).
  + The child does not inherit timers from its parent (setitimer(2), alarm(2),
    timer_create(2)).
  + The child does not inherit outstanding asynchronous I/O operations from its parent
    (aio_read(3), aio_write(3)), nor does it inherit any asynchronous I/O contexts
    from its parent (see io_setup(2)).

## [How to make child process die after parent exits?](http://stackoverflow.com/questions/284325/how-to-make-child-process-die-after-parent-exits/36945270#36945270)


