#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>

#ifndef   NI_MAXHOST
#define   NI_MAXHOST 1025
#endif

int main(void)
{
	struct addrinfo* result;
	struct addrinfo* res;
	int error;
#ifdef _WIN32
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
#endif
	/* resolve the domain name into a list of addresses */
	error = getaddrinfo("www.tired.com", NULL, NULL, &result);
	if (error != 0) {
		if (error == EAI_SYSTEM) {
			perror("getaddrinfo");
		}
		else {
			fprintf(stderr, "error in getaddrinfo: %s\n", gai_strerror(error));
		}
		exit(EXIT_FAILURE);
	}

	/* loop over all returned results and do inverse lookup */
	for (res = result; res != NULL; res = res->ai_next) {
		char hostname[NI_MAXHOST];
		error = getnameinfo(res->ai_addr, res->ai_addrlen, hostname, NI_MAXHOST, NULL, 0, 0); 
		if (error != 0) {
			fprintf(stderr, "error in getnameinfo: %s\n", gai_strerror(error));
			continue;
		}
		if (*hostname != '\0')
			printf("hostname: %s\n", hostname);
	}
#ifdef _WIN32
	WSACleanup();
#endif
	freeaddrinfo(result);
	return 0;
}
