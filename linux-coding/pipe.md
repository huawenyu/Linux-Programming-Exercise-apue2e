# Pipes Between Processes

A pipe between two processes is a pair of files that is created in a parent process. The pipe connects the resulting processes when the parent process forks. A pipe has no existence in any file name space, so it is said to be
anonymous. A pipe usually connects only two processes, although any number of child processes can be connected to each other and their related parent by a single pipe.

A pipe is created in the process that becomes the parent by a call to pipe(2). The call returns two file descriptors in the array passed to it. After forking, both processes read from p[0] and write to p[1]. The processes
actually read from and write to a circular buffer that is managed for them.

Because calling fork(2) duplicates the per-process open file table, each process has two readers and two writers. Closing the extra readers and writers enables the proper functioning of the pipe. For example, no end-of-file
indication would ever be returned if the other end of a reader is left open for writing by the same process. The following code shows pipe creation, a fork, and clearing the duplicate pipe ends.

```c
#include <stdio.h>
#include <unistd.h>

int main()
{
 //...
        int p[2];
 //...
        if (pipe(p) == -1) exit(1);
        switch( fork() )
        {
                case 0:                                         /* in child */
                        close( p[0] );
                        dup2( p[1], 1);
                        close P[1] );
                        exec( ... );
                        exit(1);
                default:                                                /* in parent */
                        close( p[1] );
                        dup2( P[0], 0 );
                        close( p[0] );
                        break;
        }
        //...
}
```

The following table shows the results of reads from a pipe and writes to a pipe, under certain conditions.

Table 5\1 Read/Write Results in a Pipe
+-----------------------------------------------------------------------+
| Attempt      | Conditions                        | Result             |
|--------------+-----------------------------------+--------------------|
| read         | Empty pipe, writer attached       | Read blocked       |
|--------------+-----------------------------------+--------------------|
| write        | Full pipe, reader attached        | Write blocked      |
|--------------+-----------------------------------+--------------------|
| read         | Empty pipe, no writer attached    | EOF returned       |
|--------------+-----------------------------------+--------------------|
| write        | No reader                         | SIGPIPE            |
+-----------------------------------------------------------------------+

Blocking can be prevented by calling fcntl(2) on the descriptor to set FNDELAY. This causes an error return (-1) from the I/O call with errno set to EWOULDBLOCK.
