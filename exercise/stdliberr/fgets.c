#include <stdio.h>   /* fopen, feof, fgets, puts */
#include <stdlib.h>  /* exit */

int main(void)
{
	FILE *fp;
	char buff[80];

	fp = fopen("file.fgets", "r");
	if (!fp) {
		perror("file.fgets not exist.");
		exit(1);
	}

	while (!feof(fp)) {                   // problem 1: it's a flag only when try to read already-end-of-file, only clear by clearerr()
		fgets(buff, sizeof(buff)-1, fp);  // problem 2: after read "line 3", it's not end, read next line (EOF) will return NULL, so should be: if (fgets())
                                          // problem 3: @param 2 should be sizeof(buff), that's enough
		puts(buff);                       // the puts will append a newline
	}
}

/* Why output like follow:
line 1

line 2

line 3

line 3

*/
