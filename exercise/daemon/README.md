<!-- START doctoc generated TOC please keep comment here to allow auto update -->
<!-- DON'T EDIT THIS SECTION, INSTEAD RE-RUN doctoc TO UPDATE -->
**Table of Contents**  *generated with [DocToc](http://doctoc.herokuapp.com/)*

- [Why need daemon?](#why-need-daemon)
- [Basic Daemon Structure](#basic-daemon-structure)
  - [Build & Run](#build-&-run)
    - [What you should see here is:](#what-you-should-see-here-is)
    - [Reading the syslog:](#reading-the-syslog)
- [Step by step](#step-by-step)
  - [1. Forking The Parent Process](#1-forking-the-parent-process)
  - [2. Changing The File Mode Mask (Umask)](#2-changing-the-file-mode-mask-umask)
  - [3. Opening Logs For Writing](#3-opening-logs-for-writing)
  - [4. Creating a Unique Session ID (SID)](#4-creating-a-unique-session-id-sid)
  - [5. Changing The Working Directory](#5-changing-the-working-directory)
  - [6. Closing Standard File Descriptors](#6-closing-standard-file-descriptors)
  - [7. Putting It All Together](#7-putting-it-all-together)
- [References](#references)

<!-- END doctoc generated TOC please keep comment here to allow auto update -->

# Why need daemon?

In Linux i want to add a daemon that cannot be stopped and which monitors filesystem changes. If any changes would be detected it should write the path to the console where it was started + a newline.

Daemons work in the background and (usually...) don't belong to a TTY that's why you can't use stdout/stderr in the way you probably want. Usually a syslog daemon (syslogd) is used for logging messages to files (debug, error,...).

A daemon (or service) is a background process that is designed to run autonomously,with little or not user intervention. The Apache web server http daemon (httpd) is one such example of a daemon. It waits in the background listening on specific ports, and serves up pages or processes scripts, based on the type of request.

Creating a daemon in Linux uses a specific set of rules in a given order. Knowing how they work will help you understand how daemons operate in userland Linux, but can operate with calls to the kernel also. In fact, a few daemons interface with kernel modules that work with hardware devices, such as external controller boards, printers,and PDAs. They are one of the fundamental building blocks in Linux that give it incredible flexibility and power.

Throughout this HOWTO, a very simple daemon will be built in C. As we go along, more code will be added, showing the proper order of execution required to get a daemon up and running.

A daemon should do one thing, and do it well. Daemons should never have direct communication with a user through a terminal. In fact, a daemon shouldn't communicate directly with a user at all. All communication should pass through some sort of interface (which you may or may not have to write), which can be as complex as a GTK+ GUI, or as simple as a signal set.

# Basic Daemon Structure

When a daemon starts up, it has to do some low-level housework to get itself ready for its real job. This involves a few steps:

- Fork off the parent process
- Change file mode mask (umask)
- Open any logs for writing
- Create a unique Session ID (SID)
- Change the current working directory to a safe place
- Close standard file descriptors
- Enter actual daemon code

> OR

- fork off the parent process & let it terminate if forking was successful. -> Because the parent process has terminated, the child process now runs in the background.
- setsid - Create a new session. The calling process becomes the leader of the new session and the process group leader of the new process group. The process is now detached from its controlling terminal (CTTY).
- Catch signals - Ignore and/or handle signals.
- fork again & let the parent process terminate to ensure that you get rid of the session leading process. (Only session leaders may get a TTY again.)
- chdir - Change the working directory of the daemon.
- umask - Change the file mode mask according to the needs of the daemon.
- close - Close all open file descriptors that may be inherited from the parent process.

To give you a starting point: Look at this skeleton code that shows the basic steps:
```c
/*
 * daemonize.c
 * This example daemonizes a process, writes a few log messages,
 * sleeps 20 seconds and terminates afterwards.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>

static void skeleton_daemon()
{
    pid_t pid;

    /* Fork off the parent process */
    pid = fork();

    /* An error occurred */
    if (pid < 0)
        exit(EXIT_FAILURE);

    /* Success: Let the parent terminate */
    if (pid > 0)
        exit(EXIT_SUCCESS);

    /* On success: The child process becomes session leader */
    if (setsid() < 0)
        exit(EXIT_FAILURE);

    /* Catch, ignore and handle signals */
    //TODO: Implement a working signal handler */
    signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP, SIG_IGN);

    /* Fork off for the second time*/
    pid = fork();

    /* An error occurred */
    if (pid < 0)
        exit(EXIT_FAILURE);

    /* Success: Let the parent terminate */
    if (pid > 0)
        exit(EXIT_SUCCESS);

    /* Set new file permissions */
    umask(0);

    /* Change the working directory to the root directory */
    /* or another appropriated directory */
    chdir("/");

    /* Close all open file descriptors */
    int x;
    for (x = sysconf(_SC_OPEN_MAX); x>0; x--)
    {
        close (x);
    }

    /* Open the log file */
    openlog ("firstdaemon", LOG_PID, LOG_DAEMON);
}
int main()
{
    skeleton_daemon();

    while (1)
    {
        //TODO: Insert daemon code here.
        syslog (LOG_NOTICE, "First daemon started.");
        sleep (20);
        break;
    }

    syslog (LOG_NOTICE, "First daemon terminated.");
    closelog();

    return EXIT_SUCCESS;
}
```

## Build & Run

```
$ gcc -o firstdaemon daemonize.c
$ ./firstdaemon
$ ps -xj | grep firstdaemon    <<< Check if everything is working properly

The output should be similar to this one:

+------+------+------+------+-----+-------+------+------+------+-----+
| PPID | PID  | PGID | SID  | TTY | TPGID | STAT | UID  | TIME | CMD |
+------+------+------+------+-----+-------+------+------+------+-----+
|    1 | 3387 | 3386 | 3386 | ?   |    -1 | S    | 1000 | 0:00 | ./  |
+------+------+------+------+-----+-------+------+------+------+-----+
```
### What you should see here is:

- The daemon has no controlling terminal (TTY = ?)
- The parent process ID (PPID) is 1 (The init process)
- The PID != SID which means that our process is NOT the session leader
  + (because of the second fork())
- Because PID != SID our process can't take control of a TTY again

### Reading the syslog:

```
$ cat /var/log/syslog
$ grep firstdaemon /var/log/syslog
  firstdaemon[3387]: First daemon started.
  firstdaemon[3387]: First daemon terminated.
```

A note: In reality you would also want to implement a signal handler and set up the logging properly (Files, log levels...).

Further reading:

[Linux-UNIX-Programmierung - German](http://openbook.galileocomputing.de/linux_unix_programmierung/Kap07-000.htm#Xxx999234)  
[Unix Daemon Server Programming](http://www.enderunix.org/docs/eng/daemon.php)  


# Step by step

## 1. Forking The Parent Process

A daemon is started either by the system itself or a user in a terminal or script. When it does start, the process is just like any other executable on the system. To make it truly autonomous, a child process must be created where the actual code is executed. This is known as forking, and it uses the fork() function:
```
        pid_t pid;

        /* Fork off the parent process */       
        pid = fork();
        if (pid < 0) {
                exit(EXIT_FAILURE);
        }
        /* If we got a good PID, then
           we can exit the parent process. */
        if (pid > 0) {
                exit(EXIT_SUCCESS);
        }
```
Notice the error check right after the call to fork(). When writing a daemon, you will have to code as defensively as possible. In fact, a good percentage of the total code in a daemon consists of nothing but error checking.

The fork() function returns either the process id (PID) of the child process (not equal to zero), or -1 on failure. If the process cannot fork a child, then the daemon should terminate right here.

If the PID returned from fork() did succeed, the parent process must exit gracefully. This may seem strange to anyone who hasn't seen it, but by forking, the child process continues the execution from here on out in the code.

## 2. Changing The File Mode Mask (Umask)

In order to write to any files (including logs) created by the daemon, the file mode mask (umask) must be changed to ensure that they can be written to or read from properly. This is similar to running umask from the command line, but we do it programmatically here. We can use the umask() function to accomplish this:
```
        /* Change the file mode mask */
        umask(0);
```
By setting the umask to 0, we will have full access to the files generated by the daemon. Even if you aren't planning on using any files, it is a good idea to set the umask here anyway, just in case you will be accessing files on the filesystem.

## 3. Opening Logs For Writing

This part is optional, but it is recommended that you open a log file somewhere in the system for writing. This may be the only place you can look for debug information about your daemon.

## 4. Creating a Unique Session ID (SID)

From here, the child process must get a unique SID from the kernel in order to operate. Otherwise, the child process becomes an orphan in the system. The pid_t type, declared in the previous section, is also used to create a new SID for the child process:
```
        /* Open any logs here */
        
        /* Create a new SID for the child process */
        sid = setsid();
        if (sid < 0) {
                /* Log any failure */
                exit(EXIT_FAILURE);
        }
```
Again, the setsid() function has the same return type as fork(). We can apply the same error-checking routine here to see if the function created the SID for the child process.

## 5. Changing The Working Directory

The current working directory should be changed to some place that is guaranteed to always be there. Since many Linux distributions do not completely follow the Linux Filesystem Hierarchy standard, the only directory that is guaranteed to be there is the root (/). We can do this using the chdir() function:
```
        /* Change the current working directory */
        if ((chdir("/")) < 0) {
                /* Log any failure here */
                exit(EXIT_FAILURE);
        }
```
Once again, you can see the defensive coding taking place. The chdir() function returns -1 on failure, so be sure to check for that after changing to the root directory within the daemon.

## 6. Closing Standard File Descriptors

One of the last steps in setting up a daemon is closing out the standard file descriptors (STDIN, STDOUT, STDERR). Since a daemon cannot use the terminal, these file descriptors are redundant and a potential security hazard.

The close() function can handle this for us:
```
        /* Close out the standard file descriptors */
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
```
It's a good idea to stick with the constants defined for the file descriptors, for the greatest portability between system versions.

## 7. Putting It All Together

Listed below is a complete sample daemon that shows all of the steps necessary for setup and execution. To run this, simply compile using gcc, and start execution from the command line. To terminate, use the kill command after finding its PID.

I've also put in the correct include statements for interfacing with the syslog, which is recommended at the very least for sending start/stop/pause/die log statements, in addition to using your own logs with the fopen()/fwrite()/fclose() function calls.
```
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>

int main(void) {
        
        /* Our process ID and Session ID */
        pid_t pid, sid;
        
        /* Fork off the parent process */
        pid = fork();
        if (pid < 0) {
                exit(EXIT_FAILURE);
        }
        /* If we got a good PID, then
           we can exit the parent process. */
        if (pid > 0) {
                exit(EXIT_SUCCESS);
        }

        /* Change the file mode mask */
        umask(0);
                
        /* Open any logs here */        
                
        /* Create a new SID for the child process */
        sid = setsid();
        if (sid < 0) {
                /* Log the failure */
                exit(EXIT_FAILURE);
        }
        

        /* Change the current working directory */
        if ((chdir("/")) < 0) {
                /* Log the failure */
                exit(EXIT_FAILURE);
        }
        
        /* Close out the standard file descriptors */
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
        
        /* Daemon-specific initialization goes here */
        
        /* The Big Loop */
        while (1) {
           /* Do some task here ... */
           
           sleep(30); /* wait 30 seconds */
        }
   exit(EXIT_SUCCESS);
}
```
From here, you can use this skeleton to write your own daemons. Be sure to add in your own logging (or use the syslog facility), and code defensively, code defensively, code defensively!

# References

[linux-daemon-howto](http://www.netzmafia.de/skripten/unix/linux-daemon-howto.html)  
[creating-a-daemon-in-linux](http://stackoverflow.com/questions/17954432/creating-a-daemon-in-linux)  

