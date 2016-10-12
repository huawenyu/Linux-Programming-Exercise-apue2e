/*
 * use and example of strstr() function
 */
/*
 * The  strstr()  function finds the first occurrence of the substring needle in the string haystack. 
 * The terminating null bytes ('\0') are
 * not compared.
 */

#include <stdio.h>
#include <string.h>
int main(){
    char *str="what the hell! system got hacked!!!";
    char *str2="what";
    char *str3="system";
    printf("\n%s\n\n",strstr(str,str3));
    printf("%s\n\n",strstr(str,str2));
    return 0;
}

/* gcc strstr1.c -o strstr1
 * ./strstr1
 * system got hacked!!!
 * 
 * what the hell! system got hacked!!!
 * 
 */
