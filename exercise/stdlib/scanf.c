/* By the way, you should be very careful with using scanf because of the potential
     to overflow your input buffer!
   Generally you should consider using fgets and sscanf rather than just scanf itself,
     using fgets to read in a line and then sscanf to parse that line as demonstrated below.
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>

/*
This code example prints the following output on the screen: 

Float 1 = 23.500000
Float 2 = -12000000.000000
Integer 1 = 100
Integer 2 = 5
*/
void test_printf_scanf(void)
{
	float f1, f2;
	int i1, i2;
	FILE *my_stream;
	char my_filename[] = "file1";

	my_stream = fopen (my_filename, "w");
	/* Note the use of the # flags in the %#d conversions in the fprintf call;
       this is a good way to generate data in a format that scanf and related functions can easily read with the %i input conversion. */
	fprintf (my_stream, "%f %f %#d %#d", 23.5, -12e6, 100, 5);

	/* Close stream; skip error-checking for brevity of example */
	fclose (my_stream);

	my_stream = fopen (my_filename, "r");
	fscanf (my_stream, "%f %f %i %i", &f1, &f2, &i1, &i2);

	/* Close stream; skip error-checking for brevity of example */
	fclose (my_stream);

	printf ("Float 1 = %f\n", f1);
	printf ("Float 2 = %f\n", f2);
	printf ("Integer 1 = %d\n", i1);
	printf ("Integer 2 = %d\n", i2);
}

/* file.scanf
line1,
line2,
line3,
Jonh Smith
Jonh Smith
Jonh Smith
 */
void test_scanf(void)
{
	FILE *fp;
	char line[100];
	char temp[32];
	char last_name[32];

	fp = fopen("file.scanf", "r");
	if (!fp) {
		perror("read 'file.scanf' fail");
		return;
	}

	if(fgets(line, sizeof(line), fp)) {
		// read until '\n', then trash that '\n'
		sscanf(line, "%[^\n]\n", temp);
		printf("trash newline{%s}\n", temp);
	}

	if(fgets(line, sizeof(line), fp)) {
		// read till a coma, also keep the coma, but test show also trash coma
		sscanf(line, "%[^,]", temp);
		printf("keep coma    {%s}\n", temp);
	}

	if(fgets(line, sizeof(line), fp)) {
		// this one trash the coma
		sscanf(line, "%[^,],",temp);
		printf("trash coma   {%s}\n", temp);
	}

	/* If you want to skip some input, use * sign after %.
	   For example you want to read last name from "John Smith"
	 */
	if(fgets(line, sizeof(line), fp)) {
		// typical answer, using 1 temporary variable
		sscanf(line, "%s %s", temp, last_name);
		printf("{%s}\n", last_name);
	}

	if(fgets(line, sizeof(line), fp)) {
		// another answer, only use 1 variable, but calls scanf twice
		sscanf(line, "%s", last_name);
		sscanf(line + strlen(last_name), "%s", last_name);
		printf("{%s}\n", last_name);
	}

	if(fgets(line, sizeof(line), fp)) {
		// best answer: don't need extra temporary variable nor calling scanf twice
		sscanf(line, "%*s %s", last_name);
		printf("{%s}\n", last_name);
	}

	fclose(fp);
}


int main()
{
	test_printf_scanf();
	test_scanf();

	return 0;
}

