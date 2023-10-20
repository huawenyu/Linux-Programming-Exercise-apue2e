
# File open describe:

https://stackoverflow.com/questions/14189944/unix-system-file-tables:
- File descriptor table,
- System open file Table: one open file table entry per open call
- Memory Inode table: one in-memory inode table entry per opened file


```python
  ┌────────────────────────────────────────────────────────────────────────────┐
  │  Userspace                                                                 │
  │                                                                            │
  │        userspace process A                 userspace process B             │
  │        ├─────────────────┐                 ├──────────────────────┐        │
  │        └─────┬───────────┘                 └──────────────┬───────┴        │
  └──────────────┼────────────────────────────────────────────┼────────────────┘
  ┌──────────────┼────────────────────────────────────────────┼────────────────┐
  │  Kernel      │                                            │                │
  │              ▼                                            ▼                │
  │  ┌──┬──┬────────────────────┬───┐   ┌─┬─┬─┬─────────────────────┬─┬─┬──┐   │
  │  │  │  │file-descript table │   │   │ │ │ │ file-descript table │ │ │  │   │
  │  └──┴──┴─────┬──────────────┴───┘   └─┴─┴─┴───────────────┬─────┴─┴─┴──┘   │
  │              │ flag: close_on_exec                        │                │
  │              │                                            │                │
  │              │         ┌─────────────────────────┐        │                │    1. Shared if dup()ed or fork()ed,
  │              └────────►│ open-file descriptor #0 │◄───────┘flag/offset(refCnt)│ 2. Every open() will create a new entry,
  │                        └─────────────────────────┘       w/r/a/b           │
  │                                                                            │
  │                                                                            │
  │                        ┌─────────────────────────┐                         │
  │                        │   file buffer/cache #0  │i-node(refCnt)           │
  │                        └─────────────────────────┘                         │
  │                                                                            │
  └────────────────────────────────────────────────────────────────────────────┘

  http://tzimmermann.org/2017/07/28/data-structures-of-unix-file-io/
  https://stackoverflow.com/questions/8175827/what-happens-if-i-dont-call-fclose-in-a-c-program
```

The fork with the purpose of calling an exec function,
you can use fcntl with FD_CLOEXEC to have the file descriptor closed once you exec:

```c
int fd = open(...);
fcntl(fd, F_SETFD, FD_CLOEXEC);
```
Such a file descriptor will survive a `fork` but not functions of the `exec` family:
- work for exec
- but not work for fork


Once your program exits, the operating system will clean up after you. It will close any
files you left open when it terminates your process, and perform any other cleanup that
is necessary (e.g. if a file was marked delete-on-close, it will delete the file then;
note that that sort of thing is platform-specific).

However, another issue to be careful of is buffered data. Most file streams buffer data in
memory before writing it out to disk. If you're using FILE* streams from the stdio library,
then there are two possibilities:

Your program exited normally, either by calling the exit(3) function, or by returning from main
(which implicitly calls exit(3)).
Your program exited abnormally; this can be via calling abort(3) or _Exit(3), dying from a
signal/exception, etc.
If your program exited normally, the C runtime will take care of flushing any buffered streams
that were open. So, if you had buffered data written to a FILE* that wasn't flushed, it will
be flushed on normal exit.

Conversely, if your program exited abnormally, any buffered data will not be flushed. The OS
just says "oh dear me, you left a file descriptor open, I better close that for you" when the
process terminates; it has no idea there's some random data lying somewhere in memory that the
program intended to write to disk but did not. So be careful about that.


## ftruncate/read/write: fopen + io-redirect-`>`

https://stackoverflow.com/questions/60950996/how-file-offset-of-read-or-write-change-when-file-is-truncated-to-zero-by-ot

> how file offset of read() or write() change when file is truncated

The file offset of opened file descriptors remains unchanged<sup>[1]</sup>.

> what happen when read() or write() after truncate().

`read()`:

- Will read valid data if the offset is in range of the file.
- Will read bytes equal to 0 if the offset is after the length of file but in the range of truncate<sup>[1]</sup>.
- Will return 0 (ie. no bytes read) if the offset is past the end of file<sup>[3]</sup>.

`write()`:

- Will write data to the file at the offset specified<sup>[4]</sup>.
- If the write is past the end-of-file, the file will be resized with padding zeros<sup>[2]</sup>.

<sup>[1]</sup> From [posix truncate](https://pubs.opengroup.org/onlinepubs/9699919799/functions/truncate.html):

> If the file previously was larger than length, the extra data is discarded. If the file was previously shorter than length, its size is increased, and the extended area appears as if it were zero-filled.
>
> The truncate() function shall not modify the file offset for any open file descriptions associated with the file.

<sup>[2]</sup> From [posix lseek](https://pubs.opengroup.org/onlinepubs/9699919799/functions/lseek.html#):

> The lseek() function shall allow the file offset to be set beyond the end of the existing data in the file. If data is later written at this point, subsequent reads of data in the gap shall return bytes with the value 0 until data is actually written into the gap.

<sup>[3]</sup> From [posix read](https://pubs.opengroup.org/onlinepubs/009695399/functions/read.html):

> No data transfer shall occur past the current end-of-file. If the starting position is at or after the end-of-file, 0 shall be returned.

<sup>[4]</sup> And from [posix write](https://pubs.opengroup.org/onlinepubs/009695399/functions/write.html):

> After a write() to a regular file has successfully returned:
>
> - Any successful read() from each byte position: in the file that was modified by that write shall return the data specified by the write() for that position until such byte positions are again modified.


```c
/**

Other comments & note:

>>> echo ---------------------
>>> echo --- TestCase1: write + redirectIO  ---
      - https://en.wikipedia.org/wiki/Control_character:
        + The NULL character (code 0) is represented by Ctrl-@
>>> echo Check content: cat -A /tmp/testlog.txt
>>> echo redirectIO:    echo "0" > /tmp/testlog.txt
>>> echo Check Offset:  lsof -o -p `pidof vim_a.out` 2&>1 | grep testlog
>>> echo Check fd stat: ls -li /proc/`pidof vim_a.out`/fd
>>> {fileout}
>>> #echo --- TestCase2: open with append mode ---
>>> #{fileout} "a+"

*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    char line[32];
    FILE *h;

    if (argc <= 1)
        h = fopen("/tmp/testlog.txt", "w+");
    else
        h = fopen("/tmp/testlog.txt", argv[1]);

    for (int i = 1; i <= 64; i++) {
            snprintf(line, sizeof(line), "%d-", i);
            if (write(fileno(h), line, strlen(line)) != strlen(line)) {
                perror("Could not append line to file");
                exit(1);
            }
            printf("Written integer %d, Press Enter to write next int, pid=%d \n", i, getpid());
            getchar();
    }


    // Show/read the file
    printf("content of this file are \n");
    fseek(h, 0, SEEK_SET);

    // Printing what is written in file
    // character by character using loop.
    int ch;
    do {
        ch = fgetc(h);
        printf("%c", ch);

        // Checking if character is not EOF.
        // If it is EOF stop reading.
    } while (ch != EOF);


    printf("Press Enter to exit, pid=%d \n", getpid());
    getchar();

    if (fclose(h) != 0) {
        perror("Could not close file");
        exit(1);
    }
    printf("parent exit\n");
    return 0;
}
```

## understand O_APPEND

https://stackoverflow.com/questions/27573677/reading-and-appending-to-the-same-file
Opening the file with the mode a+ (append, allowing reading):
- sets the file position at the beginning of the file,
  + so the first call to fgets reads the first line.
- But in append mode, all writes are performed at the end of the file.
  + So the first call to fputs sets the file position to the end of the file,
  + then writes data.
- Since there's a single file position for both reading and writing,
  + the next call to fgets is performed at the end of the file, and reads nothing.

Open file mode:
- By default the file is opened with the cursor positioned at the start. Writing overwrites the bytes at the beginning of the file.
- O_TRUNC causes the file to be truncated if it exists.
- O_APPEND causes writes to append to the end of the file instead of overwrite at the start.
  This flag is persistent, means if you move the cursor elsewhere to read data it's always repositioned to the end of the file before each write.

```c
/**

Other comments & note:

>>> echo ---------------------
>>> echo --- TestCase1: APPEND mode  ---
>>> echo Check content: cat -A 1.txt
>>> echo 1. Where is the new data will be?
>>> echo 2. Why just print out the first line?
>>> {fileout} 1.txt

*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    FILE * file_ptr;
    int num;
    char line[128];

    printf("Press Enter to continue, pid=%d \n", getpid());
    getchar();

    file_ptr = fopen(argv[1], "a+");
    while (fgets(line,128,file_ptr) != NULL) {
        printf("Read: %s", line);
        fputs("#", file_ptr);
    }

    fclose(file_ptr);
    return(0);
}
```

## sample

```c
/**

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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main()
{
	FILE *h = tmpfile();

	if (fork() == 0) {
		char* line =
			"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n";
		printf("child start write\n");
		for (int i = 0; i < 1; i++) {
			if (write(fileno(h), line, strlen(line)) != strlen(line)) {
				perror("Could not append line to file");
				exit(1);
			}
		}
		/** if (fclose(h) != 0) {
		  *         perror("Could not close file");
		  *         exit(1);
		  * } */

		printf("child exit\n");
		exit(1);
	} else {
		char* line =
			"bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb\n";
		sleep(3);
		printf("parent start write\n");
		for (int i = 0; i < 1; i++) {
			if (write(fileno(h), line, strlen(line)) != strlen(line)) {
				perror("Could not append line to file");
				exit(1);
			}
		}


		// Show/read the file
		printf("content of this file are \n");
		fseek(h, 0, SEEK_SET);

		// Printing what is written in file
		// character by character using loop.
		int ch;
		do {
			ch = fgetc(h);
			printf("%c", ch);

			// Checking if character is not EOF.
			// If it is EOF stop reading.
		} while (ch != EOF);


        printf("Press Enter to exit, pid=%d \n", getpid());
        getchar();

		if (fclose(h) != 0) {
			perror("Could not close file");
			exit(1);
		}
		printf("parent exit\n");
	}
	return 0;
}
```
