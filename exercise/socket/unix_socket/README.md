# UNIX domain sockets

## Samples

* Abstract Name as address [client](cli.c), [server](srv.c)
* Stream Sample with send file descriptor, [client](stream_cli.c), [server](stream_srv.c)
* BiDirectional DataGram Sample [client](dgram_cli.c), [server](dgram_srv.c)

## Why use unix domain socket for an IPC mechanism instead of pipes, signals, or shared memory?

* UNIX domain sockets are a method by which processes on the same host can communicate.
* Communication is bidirectional with stream sockets
* (And unidirectional with datagram sockets)
  But I test it on linux and show that datagram also support bidirectional.
* Unix domain sockets are secure in the network protocol sense of the word, because:
  - they cannot be eavesdropped on by a untrusted network
  - remote computers cannot connect to them without some sort of forwarding mechanism
* They do not require a properly configured network, or even network support at all
* They are full duplex
* Many clients can be connect to the same server using the same named socket
* Both connectionless (datagram), and connection oriented (stream) communication is supported
* Unix domain sockets are secure in the IPC sense of the word, because:
  - File permissions can be configured on the socket to limit access to certain users or groups  
    For UNIX domain sockets, file and directory permissions restrict which processes
    on the host can open the file, and thus communicate with the server.  Therefore,
    UNIX domain sockets provide an advantage over Internet sockets (to which anyone
    can connect, unless extra authentication logic is implemented).
  - Because everything that is going on takes place on the same computer controlled by a single kernel, the kernel knows everything about the socket and the parties on both sides. This means that server programs that need authentication can find out what user is connecting to them without having to obtain a user name and password.
* Linux Abstract Socket Namespace, the pathname for a UNIX domain socket begins with a null byte '\0', its name is not mapped into the filesystem. Thus it won't collide with other names in the filesystem.
* Open file descriptors from one process can be sent to another totally unrelated process
* Parties can know what PID is on the other side of a Unix domain Socket

### Abstract names for unix domain sockets:
* Another Linux specific feature is abstract names for unix domain sockets.
* The trick is to make the first byte of the address name null.
* If sun_path[0] is \0, The kernel uses the entirety of the remainder of sun_path as the name of the socket,  
  whether it's \0-terminated or not, so all of that remainder counts.
* Abstract named sockets are identical to regular UDS except that their name does not exist in the file system:
  - file permissions do not apply,
  - and they can be accessed from inside chroot() jails.
  - linux automatically "cleans up" abstract sockets to the extent that cleaning up even makes sense.

### Notes:
* The path name for a socket is limited to UNIX_PATH_MAX bytes long. On Linux, this is defined as 108 UNIX(7).
* Unlink before bind to avoid 'bind error: Address already in use'.  
  Once created, this socket file will continue to exist, even after the server exits.

### Comparison with named pipes for IPC
* bi-directional, Stream sockets provide bi-directional communication while named pipes are uni-directional.
* Distinct clients,
  - Clients using sockets each have an independent connection to the server.
  - With named pipes, many clients may write to the pipe, but the server cannot distinguish the clients from each other

