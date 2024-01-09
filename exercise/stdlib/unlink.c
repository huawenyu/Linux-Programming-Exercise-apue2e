#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(void)
{
	char filename[] = "tempfile-XXXXXX";
	int fd;
	if ((fd = mkstemp(filename)) == -1) {
		fprintf(stderr, "Failed with error %s\n", strerror(errno));
		return -1;
	}

	unlink(filename);

	FILE *fh = fdopen(fd, "w");
	fprintf(fh, "It worked!\n");
	fclose(fh);
	return 0;
}

