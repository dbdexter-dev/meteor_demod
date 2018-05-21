#ifndef _METEOR_TCP_H
#define _METEOR_TCP_H

#include <stdlib.h>

#define QUEUE_CHUNKSIZE 1024

typedef struct element {
	char data[QUEUE_CHUNKSIZE];
	struct element* next;
} Queue;

void tcp_init(char *ip, short port);
void tcp_queue_send(char *data, int size);
void tcp_deinit();

#endif
