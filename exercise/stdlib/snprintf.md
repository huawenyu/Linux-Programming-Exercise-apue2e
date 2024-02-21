
This is a sample code;
```c
#include <stdio.h>
#include <string.h>

int main()
{
	printf("Hello\n");
    return 0;
}
```

```sh
echo "Hello"
```

```c
  #include <stdio.h>
  #include <stdarg.h>
  #define BUFSIZE 9

void init_buf(char *buf, size_t size);
void print_buf(char *buf);

/**
 * @note:
 *  - ensure NULL-terminate
 *  - return the valid written characters execlude NULL-terminate,
 *  - avoid crash when buf-sz <= 0,
 *  - chain-call & ignore-error by avoid return minus value
 * @code:
    char buf[5];
    int len = 0;
    len += snprintf_s(buf+len, sizeof(buf)-len, "string1");
    len += snprintf_s(buf+len, sizeof(buf)-len, "string2");
    len += snprintf_s(buf+len, sizeof(buf)-len, "string3");
 */
static inline int snprintf_s(char *s, int n, const char *fmt, ...)
{
	int ret;
	va_list args;

	if (n <= 1)
		return 0;

	va_start(args, fmt);
	ret = vsnprintf(s, n, fmt, args);
	va_end(args);

	if (ret <= 0)
		ret = 0;
    else if (ret < n)
        return ret;
    else // return written chars if over-buff: ret >= n
        return n - 1;
}

int main()
{
    char buf[BUFSIZE];
    int ret;

    init_buf(buf, BUFSIZE);
    print_buf(buf);

    // hello there! == 12 characters, > BUFSIZE-9: ret 12 but write 8+NULL
    init_buf(buf, BUFSIZE);
    ret = snprintf(buf, BUFSIZE, "hello there!");
    printf("case1(fix) 12 > buf-9: \tret=%d ", ret);
    print_buf(buf);

    // 5 charaters, = 5:  ret 5 and write 4+NULL
    init_buf(buf, BUFSIZE);
    ret = snprintf(buf, 5, "%s", "hello");
    printf("case2(fix) 5 = buf-5: \tret=%d ", ret);
    print_buf(buf);

    // 4 charaters, = 5-1:  ret 4 and write 4+NULL
    init_buf(buf, BUFSIZE);
    ret = snprintf(buf, 5, "%s", "hell");
    printf("case3(OK) 4 = buf-5-1: \tret=%d ", ret);
    print_buf(buf);

    // turtle == 6 charaters, < BUFSIZE: ret 6 and write 6+NULL
    init_buf(buf, BUFSIZE);
    ret = snprintf(buf, BUFSIZE, "turtle");
    printf("case4(OK) 6 < buf-9: \tret=%d ", ret);
    print_buf(buf);

    // 2222220 == 7 charaters, > 5:  ret 7 and write 4+NULL
    init_buf(buf, BUFSIZE);
    ret = snprintf(buf, 5, "%d", 222222 * 10);
    printf("case5(fix) 7 > buf-5: \tret=%d ", ret);
    print_buf(buf);

    int len = 0;
    init_buf(buf, BUFSIZE);
    len += snprintf_s(buf+len, sizeof(buf)-len, "string1");
    len += snprintf_s(buf+len, sizeof(buf)-len, "string2");
    len += snprintf_s(buf+len, sizeof(buf)-len, "string3");
    printf("case6(verify) chain-call buf-9: ");
    print_buf(buf);

    return 0;
}

void init_buf(char *buf, size_t size)
{
    int i;
    for(i=0; i<size; i++){
        buf[i] = i + '0'; // int to char conversion
    }
}

void print_buf(char *buf)
{
    int i;
    char c;

    printf(" Len=%d '", strlen(buf));
    for(i=0; i<BUFSIZE; i++){
        c = buf[i];
        if(c == '\0'){
            printf("\\0");

        }
        else{
            printf("%c", buf[i]);
        }
    }
    printf("'\n");
}


```
