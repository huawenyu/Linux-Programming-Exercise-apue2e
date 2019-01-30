# Content List

There have compare [select/poll/epoll](COMPARE.md)

## sample
  - [select](select.c),
  - [pselect](pselect.c)
  - [poll](poll.c)
  - [epoll](epoll.c)
  - [unix-domain-socket](unix_socket)
  - [getaddrinfo](getaddrinfo.c)  
    [source code of getaddrinfo](http://opensource.apple.com//source/passwordserver_sasl/passwordserver_sasl-14/cyrus_sasl/lib/getaddrinfo.c)

But we should know that:
  - select/poll have [signal race condition](RACE.md) issue, [sample here](select_issue.c)
  - which can be solved by [pselect](pselect.c)/ppoll.

# Conceptions

## EAGAIN & EINTR

### EAGAIN: only for non-blocking fd
Linux, EAGAIN is same as EWOULDBLOCK

    #define EAGAIN EWOULDBLOCK

在Linux环境下开发经常会碰到很多错误(设置errno)，其中EAGAIN是其中比较常见的一个错误(比如用在非阻塞操作中)。
    从字面上来看，是提示再试一次。这个错误经常出现在当应用程序进行一些非阻塞(non-blocking)操作(对文件或socket)的时候。例如，以O_NONBLOCK的标志打开文件/socket/FIFO，如果你连续做read操作而没有数据可读。此时程序不会阻塞起来等待数据准备就绪返回，read函数会返回一个错误EAGAIN，提示你的应用程序现在没有数据可读请稍后再试。
    又例如，当一个系统调用(比如fork)因为没有足够的资源(比如虚拟内存)而执行失败，返回EAGAIN提示其再调用一次(也许下次就能成功)。

Linux - 非阻塞socket编程处理EAGAIN错误
　在linux进行非阻塞的socket接收数据时经常出现Resource temporarily unavailable，errno代码为11(EAGAIN)，这是什么意思？
　这表明你在非阻塞模式下调用了阻塞操作，在该操作没有完成就返回这个错误，这个错误不会破坏socket的同步，不用管它，下次循环接着recv就可以。对非阻塞socket而言，EAGAIN不是一种错误。在VxWorks和Windows上，EAGAIN的名字叫做EWOULDBLOCK。


### EINTR：system catch a signal

慢系统调用(slow system call)：此术语适用于那些可能永远阻塞的系统调用。永远阻塞的系统调用是指调用有可能永远无法返回，多数网络支持函数都属于这一类。如：若没有客户连接到服务器上，那么服务器的accept调用就没有返回的保证。

EINTR错误的产生：当阻塞于某个慢系统调用的一个进程捕获某个信号且相应信号处理函数返回时，该系统调用可能返回一个EINTR错误。例如：在socket服务器端，设置了信号捕获机制，有子进程，当在父进程阻塞于慢系统调用时由父进程捕获到了一个有效信号时，内核会致使accept返回一个EINTR错误(被中断的系统调用)。

当碰到EINTR错误的时候，可以采取有一些可以重启的系统调用要进行重启，而对于有一些系统调用是不能够重启的。例如：accept、read、write、select、和open之类的函数来说，是可以进行重启的。不过对于套接字编程中的connect函数我们是不能重启的，若connect函数返回一个EINTR错误的时候，我们不能再次调用它，否则将立即返回一个错误。针对connect不能重启的处理方法是，必须调用select来等待连接完成。

对于socket接口(指connect/send/recv/accept..等等后面不重复，不包括不能设置非阻塞的如select)，在阻塞模式下有可能因为发生信号，返回EINTR错误，由用户做重试或终止。

但是，在非阻塞模式下，是否出现这种错误呢？
对此，重温了系统调用、信号、socket相关知识，得出结论是：不会出现。

首先，
1.信号的处理是在用户态下进行的，也就是必须等待一个系统调用执行完了才会执行进程的信号函数，所以就有了信号队列保存未执行的信号
2.用户态下被信号中断时，内核会记录中断地址，信号处理完后，如果进程没有退出则重回这个地址继续执行

socket接口是一个系统调用，也就是即使发生了信号也不会中断，必须等socket接口返回了，进程才能处理信号。
也就是，EINTR错误是socket接口主动抛出来的，不是内核抛的。socket接口也可以选择不返回，自己内部重试之类的..

那阻塞的时候socket接口是怎么处理发生信号的?

举例
socket接口，例如recv接口会做2件事情，
1.检查buffer是否有数据，有则复制清除返回
2.没有数据，则进入睡眠模式，当超时、数据到达、发生错误则唤醒进程处理

socket接口的实现都差不了太多，抽象说
1.资源是否立即可用，有则返回
2.没有，就等...

对于
1.这个时候不管有没信号，也不返回EINTR，只管执行自己的就可以了
2.采用睡眠来等待，发生信号的时候进程会被唤醒，socket接口唤醒后检查有无未处理的信号(signal_pending)会返回EINTR错误。

所以
socket接口并不是被信号中断，只是调用了睡眠，发生信号睡眠会被唤醒通知进程，然后socket接口选择主动退出，这样做可以避免一直阻塞在那里，有退出的机会。非阻塞时不会调用睡眠。

