#include <stdio.h>
#include <stdlib.h>

inline int fun()
{
	return 222;
}

int bar()
{
	printf("inline2: fun()=%d &fun=%p\n", fun(),  (void*)&fun);
}

