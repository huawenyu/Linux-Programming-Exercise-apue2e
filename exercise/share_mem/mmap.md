# mmap: share-memory, malloc

> Description.
> [More information](https://url-to-upstream.tld).

# Practice
  - data-channel: ft_scbuf_open/ft_scbuf_write/ft_scbuf_read
    + require command-channel co-op like ftp, :)


# Features:
! [This is hidden comment](https://github.com/huawenyu/dotfiles/script/tldr)
The feature list:
  - support multiple page sources
  - support edit(vi) mode by option `-e`
  - support mutiple syntax: block, fence, list, comment

```c
// https://linuxhint.com/using_mmap_function_linux/
//
// mmap used as malloc
//
#include <stdio.h>
#include <sys/mman.h>

int main(){

	int N=5;
	int *ptr = mmap ( NULL, N*sizeof(int),
			  PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0 );

	if(ptr == MAP_FAILED){
		printf("Mapping Failed\n");
		return 1;
	}

	for(int i=0; i<N; i++)
		ptr[i] = i*10;

	for(int i=0; i<N; i++)
		printf("[%d] ",ptr[i]);

	printf("\n");
	int err = munmap(ptr, 10*sizeof(int));
	if(err != 0){
		printf("UnMapping Failed\n");
		return 1;
	}

	return 0;
}

```


```c
// https://linuxhint.com/using_mmap_function_linux/
//
// mmap used to read a file
//
#include <stdio.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char *argv[]){

	if (argc < 2) {
		printf("File path not mentioned\n");
		exit(0);
	}

	const char *filepath = argv[1];
	int fd = open(filepath, O_RDONLY);
	if (fd < 0){
		perror("\n\"%s \" could not open\n", filepath);
		exit(1);
	}

	struct stat statbuf;
	int err = fstat(fd, &statbuf);
	if (err < 0){
		perror("\n\"%s \" could not open\n", filepath);
		exit(2);
	}

	char *ptr = mmap(NULL,statbuf.st_size,
			 PROT_READ|PROT_WRITE,MAP_SHARED,
			 fd,0);
	if (ptr == MAP_FAILED){
		perror("Mapping Failed\n");
		return 1;
	}
	close(fd);

	ssize_t n = write(1,ptr,statbuf.st_size);
	if (n != statbuf.st_size){
		perror("Write failed");
	}

	err = munmap(ptr, statbuf.st_size);
	if (err != 0){
		perror("UnMapping Failed\n");
		return 1;
	}
	return 0;
}
```

```c
/*
https://linuxhint.com/using_mmap_function_linux/
mmap used to Writing file

size-of
     file   map  write unmap
     1024  2048  2048  2048

>>> {fileout}  1024  2048  2048  2048
>>> echo Normal

>>> {fileout}  1024  2048  2048  3048
>>> echo OK: unmap more size

>>> {fileout}  1024  2048  2048  5048
>>> echo ERROR: unmap > 4K page (Segmentation-fault)

>>> {fileout}  1024  2048  4096  2048
>>> echo OK: write < 4K-page

>>> {fileout}  1024  2048  4097  2048
>>> echo ERROR: write > 4K page (Bus error)

>>> size {fileout}
>>> ls -l {file}
*/

#include <stdio.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
	const char *filepath = "/tmp/no_use_map.txt";
	int file_size, map_size, write_size, unmap_size;
	int ret = 0;

	file_size  = atoi(argv[1]);
	map_size   = atoi(argv[3]);
	write_size = atoi(argv[3]);
	unmap_size = atoi(argv[4]);

	unlink(filepath);
	int fd = open(filepath, O_RDWR | O_CREAT);
	if (fd < 0) {
		perror("file open fail");
		goto out;
	}

	struct stat statbuf;
	int err = fstat(fd, &statbuf);
	if (err < 0) {
		perror("file fstat fail");
		goto out;
	}
	// statbuf.st_size

	ftruncate(fd, file_size);
	char *ptr = mmap(NULL, map_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	if (ptr == MAP_FAILED) {
		perror("Mapping Failed\n");
		ret = 1;
	}
	close(fd);

	for (int i=0; i < write_size; i++) {
		ptr[i] = 'A';
	}

	err = munmap(ptr, unmap_size);
	if (err != 0) {
		perror("UnMapping Failed\n");
		ret = 1;
	}

out:
	unlink(filepath);
	return ret;
}
```

```c
// https://linuxhint.com/using_mmap_function_linux/
//
// mmap used to Interprocess communication
//

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>

int main()
{
	int N = 5; // Number of elements for the array

	int *ptr = mmap(NULL, N*sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0,0);
	if (ptr == MAP_FAILED) {
		perror("Mapping Failed\n");
		return 1;
	}

	for (int i=0; i < N; i++) {
		ptr[i] = i + 1;
	}

	printf("Initial values of the array elements :\n");
	for (int i = 0; i < N; i++) {
		printf(" %d", ptr[i] );
	}
	printf("\n");

	pid_t child_pid = fork();
	if (child_pid == 0) {
		//child
		for (int i = 0; i < N; i++) {
			ptr[i] = ptr[i] * 10;
		}
	} else {
		//parent
		waitpid ( child_pid, NULL, 0);
		printf("\nParent:\n");

		printf("Updated values of the array elements :\n");
		for (int i = 0; i < N; i++ ){
			printf(" %d", ptr[i] );
		}
		printf("\n");
	}

	int err = munmap(ptr, N*sizeof(int));
	if (err != 0){
		perror("UnMapping Failed\n");
		return 1;
	}

	return 0;
}
```

