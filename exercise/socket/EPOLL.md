# Doc

http://blog.csdn.net/liuxuejiang158blog/article/details/12422471
http://blog.csdn.net/liuxuejiang158blog/article/details/12290725
http://www.serverphorums.com/read.php?12,628000
http://stackoverflow.com/questions/12344755/epolloneshot-returns-multiple-event
http://stackoverflow.com/questions/2673890/epoll-in-epollet-mode-returning-2-epollin-before-reading-from-the-socket

http://www.alexonlinux.com/signal-handling-in-linux
http://www.ibm.com/support/knowledgecenter/en/ssw_i5_54/rzab6/example.htm
http://stackoverflow.com/questions/9774986/linux-select-vs-ppoll-vs-pselect

# How to use epoll

[A complete example in C][6]

Network servers are traditionally implemented using a separate process or thread per connection. For high performance applications that need to handle a very large number of clients simultaneously, this approach won't work well, because factors such as resource usage and context-switching time influence the ability to handle many clients at a time. An alternate method is to perform [non-blocking I/O][8] in a single thread, along with some readiness notification method which tells you when you can read or write more data on a socket.

This article is an introduction to Linux's **epoll**(7) facility, which is the best readiness notification facility in Linux. We will write sample code for a complete TCP server implementation in C. I assume you have C programming experience, know how to compile and run programs on Linux, and can read manpages of the various C functions that are used.

epoll was introduced in Linux 2.6, and is not available in other UNIX-like operating systems. It provides a facility similar to the **select**(2) and **poll**(2) functions:

* **select**(2) can monitor up to `FD_SETSIZE` number of descriptors at a time, typically a small number determined at libc's compile time.
* **poll**(2) doesn't have a fixed limit of descriptors it can monitor at a time, but apart from other things, even we have to perform a linear scan of all the passed descriptors every time to check readiness notification, which is O(n) and slow.

epoll has no such fixed limits, and does not perform any linear scans. Hence it is able to perform better and handle a larger number of events.

An epoll instance is created by **epoll_create**(2) or **epoll_create1**(2) (they take different arguments), which return an epoll instance. **epoll_ctl**(2) is used to add/remove descriptors to be watched on the epoll instance. To wait for events on the watched set, **epoll_wait**(2) is used, which blocks until events are available. Please see their manpages for more info.

When descriptors are added to an epoll instance, they can be added in two modes: **level triggered** and **edge triggered.** When you use level triggered mode, and data is available for reading, **epoll_wait**(2) will always return with ready events. If you don't read the data completely, and call **epoll_wait**(2) on the epoll instance watching the descriptor again, it will return again with a ready event because data is available. In edge triggered mode, you will only get a readiness notfication once. If you don't read the data fully, and call **epoll_wait**(2) on the epoll instance watching the descriptor again, it will block because the readiness event was already delivered.

The epoll event structure that you pass to **epoll_ctl**(2) is shown below. With every descriptor being watched, you can associate an integer or a pointer as user data.
```C
    typedef union epoll_data
    {
      void        *ptr;
      int          fd;
      __uint32_t   u32;
      __uint64_t   u64;
    } epoll_data_t;

    struct epoll_event
    {
      __uint32_t   events; /* Epoll events */
      epoll_data_t data;   /* User data variable */
    };
```
Let's write code now. We'll implement a tiny TCP server that prints everything sent to the socket on standard output. We'll begin by writing a function **create_and_bind**() which creates and binds a TCP socket:
```C
    static int
    create_and_bind (char *port)
    {
      struct addrinfo hints;
      struct addrinfo *result, *rp;
      int s, sfd;

      memset (&hints, 0, sizeof (struct addrinfo));
      hints.ai_family = AF_UNSPEC;     /* Return IPv4 and IPv6 choices */
      hints.ai_socktype = SOCK_STREAM; /* We want a TCP socket */
      hints.ai_flags = AI_PASSIVE;     /* All interfaces */

      s = getaddrinfo (NULL, port, &hints, &result);
      if (s != 0)
        {
          fprintf (stderr, "getaddrinfo: %sn", gai_strerror (s));
          return -1;
        }

      for (rp = result; rp != NULL; rp = rp->ai_next)
        {
          sfd = socket (rp->ai_family, rp->ai_socktype, rp->ai_protocol);
          if (sfd == -1)
            continue;

          s = bind (sfd, rp->ai_addr, rp->ai_addrlen);
          if (s == 0)
            {
              /* We managed to bind successfully! */
              break;
            }

          close (sfd);
        }

      if (rp == NULL)
        {
          fprintf (stderr, "Could not bindn");
          return -1;
        }

      freeaddrinfo (result);

      return sfd;
    }
```
**create_and_bind**() contains a standard code block for a portable way of getting a IPv4 or IPv6 socket. It accepts a `port` argument as a string, where `argv[1]` can be passed. The **getaddrinfo**(3) function returns a bunch of `addrinfo` structures in `result`, which are compatible with the hints passed in the `hints` argument. The `addrinfo` struct looks like this:
```C
    struct addrinfo
    {
      int              ai_flags;
      int              ai_family;
      int              ai_socktype;
      int              ai_protocol;
      size_t           ai_addrlen;
      struct sockaddr *ai_addr;
      char            *ai_canonname;
      struct addrinfo *ai_next;
    };
```
We walk through the structures one by one and try creating sockets using them, until we are able to both create and bind a socket. If we were successful, **create_and_bind**() returns the socket descriptor. If unsuccessful, it returns -1.

Next, let's write a function to make a socket non-blocking. **make_socket_non_blocking**() sets the `O_NONBLOCK` flag on the descriptor passed in the `sfd` argument:
```C
    static int
    make_socket_non_blocking (int sfd)
    {
      int flags, s;

      flags = fcntl (sfd, F_GETFL, 0);
      if (flags == -1)
        {
          perror ("fcntl");
          return -1;
        }

      flags |= O_NONBLOCK;
      s = fcntl (sfd, F_SETFL, flags);
      if (s == -1)
        {
          perror ("fcntl");
          return -1;
        }

      return 0;
    }
```
Now, on to the **main**() function of the program which contains the event loop. This is the bulk of the program:
```C
    #define MAXEVENTS 64

    int
    main (int argc, char *argv[])
    {
      int sfd, s;
      int efd;
      struct epoll_event event;
      struct epoll_event *events;

      if (argc != 2)
        {
          fprintf (stderr, "Usage: %s [port]n", argv[0]);
          exit (EXIT_FAILURE);
        }

      sfd = create_and_bind (argv[1]);
      if (sfd == -1)
        abort ();

      s = make_socket_non_blocking (sfd);
      if (s == -1)
        abort ();

      s = listen (sfd, SOMAXCONN);
      if (s == -1)
        {
          perror ("listen");
          abort ();
        }

      efd = epoll_create1 (0);
      if (efd == -1)
        {
          perror ("epoll_create");
          abort ();
        }

      event.data.fd = sfd;
      event.events = EPOLLIN | EPOLLET;
      s = epoll_ctl (efd, EPOLL_CTL_ADD, sfd, &event);
      if (s == -1)
        {
          perror ("epoll_ctl");
          abort ();
        }

      /* Buffer where events are returned */
      events = calloc (MAXEVENTS, sizeof event);

      /* The event loop */
      while (1)
        {
          int n, i;

          n = epoll_wait (efd, events, MAXEVENTS, -1);
          for (i = 0; i < n; i++)
    	{
    	  if ((events[i].events & EPOLLERR) ||
                  (events[i].events & EPOLLHUP) ||
                  (!(events[i].events & EPOLLIN)))
    	    {
                  /* An error has occured on this fd, or the socket is not
                     ready for reading (why were we notified then?) */
    	      fprintf (stderr, "epoll errorn");
    	      close (events[i].data.fd);
    	      continue;
    	    }

    	  else if (sfd == events[i].data.fd)
    	    {
                  /* We have a notification on the listening socket, which
                     means one or more incoming connections. */
                  while (1)
                    {
                      struct sockaddr in_addr;
                      socklen_t in_len;
                      int infd;
                      char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];

                      in_len = sizeof in_addr;
                      infd = accept (sfd, &in_addr, &in_len);
                      if (infd == -1)
                        {
                          if ((errno == EAGAIN) ||
                              (errno == EWOULDBLOCK))
                            {
                              /* We have processed all incoming
                                 connections. */
                              break;
                            }
                          else
                            {
                              perror ("accept");
                              break;
                            }
                        }

                      s = getnameinfo (&in_addr, in_len,
                                       hbuf, sizeof hbuf,
                                       sbuf, sizeof sbuf,
                                       NI_NUMERICHOST | NI_NUMERICSERV);
                      if (s == 0)
                        {
                          printf("Accepted connection on descriptor %d "
                                 "(host=%s, port=%s)n", infd, hbuf, sbuf);
                        }

                      /* Make the incoming socket non-blocking and add it to the
                         list of fds to monitor. */
                      s = make_socket_non_blocking (infd);
                      if (s == -1)
                        abort ();

                      event.data.fd = infd;
                      event.events = EPOLLIN | EPOLLET;
                      s = epoll_ctl (efd, EPOLL_CTL_ADD, infd, &event);
                      if (s == -1)
                        {
                          perror ("epoll_ctl");
                          abort ();
                        }
                    }
                  continue;
                }
              else
                {
                  /* We have data on the fd waiting to be read. Read and
                     display it. We must read whatever data is available
                     completely, as we are running in edge-triggered mode
                     and won't get a notification again for the same
                     data. */
                  int done = 0;

                  while (1)
                    {
                      ssize_t count;
                      char buf[512];

                      count = read (events[i].data.fd, buf, sizeof buf);
                      if (count == -1)
                        {
                          /* If errno == EAGAIN, that means we have read all
                             data. So go back to the main loop. */
                          if (errno != EAGAIN)
                            {
                              perror ("read");
                              done = 1;
                            }
                          break;
                        }
                      else if (count == 0)
                        {
                          /* End of file. The remote has closed the
                             connection. */
                          done = 1;
                          break;
                        }

                      /* Write the buffer to standard output */
                      s = write (1, buf, count);
                      if (s == -1)
                        {
                          perror ("write");
                          abort ();
                        }
                    }

                  if (done)
                    {
                      printf ("Closed connection on descriptor %dn",
                              events[i].data.fd);

                      /* Closing the descriptor will make epoll remove it
                         from the set of descriptors which are monitored. */
                      close (events[i].data.fd);
                    }
                }
            }
        }

      free (events);

      close (sfd);

      return EXIT_SUCCESS;
    }
```
**main**() first calls **create_and_bind**() which sets up the socket. It then makes the socket non-blocking, and then calls **listen**(2). It then creates an epoll instance in `efd`, to which it adds the listening socket `sfd` to watch for input events in an edge-triggered mode.

The outer while loop is the main events loop. It calls **epoll_wait**(2), where the thread remains blocked waiting for events. When events are available, **epoll_wait**(2) returns the events in the `events` argument, which is a bunch of `epoll_event` structures.

The epoll instance in `efd` is continuously updated in the event loop when we add new incoming connections to watch, and remove existing connections when they die.

When events are available, they can be of three types:

* **Errors:** When an error condition occurs, or the event is not a notification about data available for reading, we simply close the associated descriptor. Closing the descriptor automatically removes it from the watched set of epoll instance `efd`.
* **New connections:** When the listening descriptor `sfd` is ready for reading, it means one or more new connections have arrived. While there are new connections, **accept**(2) the connections, print a message about it, make the incoming socket non-blocking and add it to the watched set of epoll instance `efd`.
* **Client data:** When data is available for reading on any of the client descriptors, we use **read**(2) to read the data in pieces of 512 bytes in an inner while loop. This is because we have to read all the data that is available now, as we won't get further events about it as the descriptor is watched in edge-triggered mode. The data which is read is written to stdout (fd=1) using **write**(2). If **read**(2) returns 0, it means an EOF and we can close the client's connection. If -1 is returned, and `errno` is set to `EAGAIN`, it means that all data for this event was read, and we can go back to the main loop.

That's that. It goes around and around in a loop, adding and removing descriptors in the watched set.

Download the [**epoll-example.c**][9] program.

**Update1:** Level and edge triggered definitions were erroneously reversed (though the code was correct). It was noticed by Reddit user [bodski][10]. The article has been corrected now. I should have proof-read it before posting. Apologies, and thank you for pointing out the mistake. :)

**Update2:** The code has been modified to run **accept**(2) until it says it would block, so that if multiple connections have arrived, we accept all of them. It was noticed by Reddit user [cpitchford][11]. Thank you for the comments. :)

[1]: /
[2]: /blog/
[3]: /products/
[4]: /about/
[5]: https://twitter.com/banubears
[6]: /blog/2/how-to-use-epoll-a-complete-example-in-c/
[7]: https://twitter.com/muks
[8]: http://en.wikipedia.org/wiki/Asynchronous_I/O
[9]: /blog/2/how-to-use-epoll-a-complete-example-in-c/epoll-example.c
[10]: http://www.reddit.com/user/bodski
[11]: http://www.reddit.com/user/cpitchford
