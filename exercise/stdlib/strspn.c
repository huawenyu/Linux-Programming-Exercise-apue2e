/* The use and example of strspn(s, accept), strcspn(s, reject)
   strspn, foreach s and find first char not on accept-char-set, return the pos.
   strcspn, foreach s and find first char in reject-char-set, return the pos.
   if not find, return the strlen(s)
   */

#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[])
{
	char *str1 = "1478abc";
	char *str2 = "0123456789";
	char *accept = "0123456789";

	char *str3 = "gdfa1234af5";
	char *reject = "ha";

	printf("%s in %s is %zu\n", str1, accept, strspn(str1, accept)); // 4
	printf("%s in %s is %zu\n", str2, accept, strspn(str2, accept)); // 10
	printf("%s in %s is %zu\n", str3, reject, strcspn(str3, reject)); // 3

	return 0;
}

/** output:

1478abc in 0123456789 is 4
0123456789 in 0123456789 is 10
gdfa1234af5 in ha is 3
*/
