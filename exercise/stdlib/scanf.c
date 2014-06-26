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
void test_scanf_number(void)
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
void test_scanf_string(void)
{
	FILE *fp;
	int len;
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
		sscanf(line, "%[^\n]\n%n", temp, &len);
		printf("trash newline{%s} read-len %d\n", temp, len);
	}

	if(fgets(line, sizeof(line), fp)) {
		// read till a coma, also keep the coma
		sscanf(line, "%[^,]%n", temp, &len);
		printf("keep coma    {%s} read-len %d\n", temp, len);
	}

	if(fgets(line, sizeof(line), fp)) {
		// this one trash the coma
		sscanf(line, "%[^,],%n",temp, &len);
		printf("trash coma   {%s} read-len %d\n", temp, len);
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

void test_scanf_buff(void)
{
	char line[256] = {0};
	char name[16];
	int age;
	float wage;

	sprintf(line, "james 20 34000.25");
	sscanf(line, "%s %d %f", name, &age, &wage);
	printf("name %s age %d wage %f\n", name, age, wage);

	sprintf(line, "james/20/34000.25");
	/*
	%[]类似于一个正则表达式。[a-z]表示读取a-z的所有字符，[^a-z]表示读取除a-z以外的所有字符。

	%[a-z] 表示匹配a到z中任意字符，贪婪性(尽可能多的匹配)
	%[aB'] 匹配a、B、'中一员，贪婪性
	%[^a] 匹配非a的任意字符，贪婪性

	- specify match-stop chars %[^<chars>]: The first conversion specification is a scan set that accepts a sequence of non-blanks, non-slashes (so it will stop at the first blank or slash). It would be best to specify an upper bound on how many characters will be accepted so as to avoid stack overflow; for example, if char name[32];, then %31[^ /] (note the off-by-one).
	- suppressing character, skip variable: The second conversion specification %*1[ /] accepts a single character (1) that is either a blank or slash [ /], and does not assign it to any variable (*).
	- The third conversion specification is a standard numeric input, skipping leading blanks, allowing for negative numbers to be entered, etc.
	- The fourth conversion specification is the same as the second,
	- and the fifth is a standard format for a float (which means that 34000.25 with 7 significant digits is at the outer end of the range of representable values).

	Note that the 'something went wrong' part has a difficult time reporting the error coherently to the user.
	This is why many people, myself included, recommend against using scanf() or fscanf()
	  and prefer to use fgets() or perhaps POSIX getline to read a line from the user and then use sscanf() to analyze it.
	You can report the problems much more easily.
	Also note that the return value from scanf() is the number of successful assignments;
	it does not count the conversion specifications that include *.
	*/
	if (sscanf(line, "%[^ /]%*1[ /]%d%*1[ /]%f", name, &age, &wage) == 3)
		printf("name %s age %d wage %f\n", name, age, wage);
	else
		printf("name age wage failed\n");
}

/* 假定我们读取一批商品记录，每条记录包含商品ID，商品名称，商品价格，各字段的类型在代码中是自包含的

- %*s 用来扫描但跳过（不存储）一个字符串，两个 %n，前一个得到name的起始偏移，后一个得到name的结束偏移……其它的，也就不用我多说了。
- %n 应该是 C99 新加入的，不过这一点我不太确信，但 gnu 里面的确有这个实现，msvc 里面没有，其他的环境就不太清楚了。如果是写服务器应用，一般就不用担心它是否实现（95%以上的服务器都该有gnu环境吧，况且 glibc 和gcc是相对独立的）。
- %n 特性真可谓小技巧，大智慧，我很长一段时间不知道这个东西，还一直为 XXprintf 系列可以得到已写出的字节数，而 XXscanf 无法得到已读取的字节数犯愁。
- 我觉得在新代码中应该使用该技巧取代 %s，当然，gnu 的 XXscanf 里面还有个 %as，它返回一个由系统 malloc 出来的字符串，需要用户自己调用 free 来释放它。如果是写纯 C 代码，并且字符串需要保存起来长期使用，应该使用 %as，否则应该使用我提到的这个技巧。
*/
void test_scanf_return_readlen(void)
{
	char line[256] = {0};
	sprintf(line, "4123 football 23.49");
	int beg = -1; // name offset begin in line
	int end = -1; // name offset end   in line
	int id;
	const char* name;
	double price;
	int fields = sscanf(line, "%d %n%*s%n %lf", &id, &beg, &end, &price);
	if (2 == fields) { // 'beg' and 'end' are not counted in return value
		int slen = end - beg;
		printf("ID %d name %*s price %f\n", id, slen, &line[beg], price);
	} else {
		printf("bad record\n");
	}


	// use readlen to read field one by one
	int nums_now, bytes_now;
	int bytes_consumed = 0, nums_read = 0;

	sprintf(line, "10 20 30 40 50 60");
	while (0 < (nums_now=sscanf(line + bytes_consumed, "%d%n", &nums_read, &bytes_now))) {
		printf("read number is %d\n", nums_read);
		bytes_consumed += bytes_now;
		nums_read += nums_now;
	}
}

int main()
{
	test_scanf_number();
	test_scanf_string();
	test_scanf_buff();
	test_scanf_return_readlen();

	return 0;
}

