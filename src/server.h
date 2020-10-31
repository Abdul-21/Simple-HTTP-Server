#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <strings.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>

#define LINES 1024
#define BUFSIZE 8196

struct threadArg {
	int clientfd;
};
