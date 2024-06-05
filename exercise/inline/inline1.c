#include <stdio.h>
#include <stdlib.h>
#include "inline1.h"

extern void bar();

extern inline int fun()
{
	return 111;
}

int main()
{
	printf("inline1: fun0=%d fun()=%d &fun=%p\n", fun0(), fun(),  (void*)&fun);
	bar();
}

