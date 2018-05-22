#include <arpa/inet.h>
#include <assert.h>
#include <netinet/in.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include "tcp.h"
#include "tui.h"
#include "utils.h"

#define DEF_PORT 2011

static void* tcp_thread_listen(void *server_sock);
static int _running;
static pthread_mutex_t _queue_tex;
static sem_t _queue_sem;
static Queue *_queue;
static pthread_t _t;

void
tcp_init(char *ip, short port)
{
	size_t server_sock;
	int err;
	const int one = 1;
	struct sockaddr_in server;

	if (!port) {
		port = DEF_PORT;
	}

	server_sock = socket(AF_INET, SOCK_STREAM, 0);
	assert(server_sock >= 0);

	server.sin_family = AF_INET;
	server.sin_port = htons(port);

	if (ip) {
		assert(inet_pton(AF_INET, ip, &(server.sin_addr)) == 1);
	} else {
		server.sin_addr.s_addr = htonl(INADDR_ANY);
	}

	setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
	err = bind(server_sock, (struct sockaddr*)&server, sizeof(server));
	assert(err == 0);

	tui_print_info("Server listening on port %d\n", port);
	_running = 1;

	pthread_mutex_init(&_queue_tex, NULL);
	sem_init(&_queue_sem, 0, 0);

	_queue = NULL;

	/* Start server listeting thread */
	assert(!pthread_create(&_t, NULL, tcp_thread_listen, (void*)server_sock));
}

void
tcp_queue_send(char *data, int size)
{
	Queue *cur;

	/* _queue has not been allocated, handle the case separately */
	if (!_queue) {
		pthread_mutex_lock(&_queue_tex);

		_queue = safealloc(sizeof(*_queue));
		_queue->next = NULL;
		if (size < QUEUE_CHUNKSIZE) {
			memset(_queue->data, '\0', QUEUE_CHUNKSIZE);
		}
		memcpy(_queue->data, data, MIN(size, QUEUE_CHUNKSIZE));
		sem_post(&_queue_sem);
		data += QUEUE_CHUNKSIZE;
		size -= QUEUE_CHUNKSIZE;

		pthread_mutex_unlock(&_queue_tex);
	}

	if (!size) {
		return;
	}
	/* Get to the tail of the queue */
	for (cur = _queue; cur->next != NULL; cur = cur->next)
	;

	/* Copy the data on the queue one chunk at a time */
	pthread_mutex_lock(&_queue_tex);
	while (size > 0) {
		cur->next = safealloc(sizeof(*cur));
		cur = cur->next;
		cur->next = NULL;

		if (size < QUEUE_CHUNKSIZE) {
			memset(cur->data, '\0', QUEUE_CHUNKSIZE);
		}
		memcpy(cur->data, data, MIN(size, QUEUE_CHUNKSIZE));

		sem_post(&_queue_sem);
		data += QUEUE_CHUNKSIZE;
		size -= QUEUE_CHUNKSIZE;
	}
	pthread_mutex_unlock(&_queue_tex);
}

/* Stop the child thread and deinit the TCP server */
void
tcp_deinit()
{
	void* ret;
	_running = 0;
	tui_print_info("Waiting for children to terminate...\n");
	pthread_join(_t, &ret);

	pthread_mutex_destroy(&_queue_tex);
	sem_destroy(&_queue_sem);
}

/* Static functions {{{*/
void*
tcp_thread_listen(void *server_sock)
{
	Queue *tmp;
	int client_sock;
	socklen_t client_len;
	struct sockaddr_in client;

	client_len = sizeof(client);

	/* Listen for possible clients */
	while (_running) {
		listen((size_t)server_sock, 1);
		client_sock = accept((size_t)server_sock, (struct sockaddr*)&client, &client_len);
		tui_print_info("Accepted client connection, fd = %d\n", client_sock);

		/* Server-client communication */
		while (_running) {
			sem_wait(&_queue_sem);
			pthread_mutex_lock(&_queue_tex);

			if (send(client_sock, _queue->data, QUEUE_CHUNKSIZE, MSG_NOSIGNAL) == -1) {
				tui_print_info("Lost connection to client, fd = %d\n", client_sock);
				pthread_mutex_unlock(&_queue_tex);
				break;
			}

			tmp = _queue;
			_queue = _queue->next;
			free(tmp);

			pthread_mutex_unlock(&_queue_tex);
		}

		/* Do this only if we legitimately exited from the while loop above, not
		 * if the client closed the connection abruptly */
		if (!_running) {
			/* Empty the queue */
			pthread_mutex_lock(&_queue_tex);
			while (_queue) {
				send(client_sock, _queue->data, QUEUE_CHUNKSIZE, MSG_NOSIGNAL);
				tmp = _queue;
				_queue = _queue->next;
				free(tmp);
			}
			pthread_mutex_unlock(&_queue_tex);

			/* Gracefully close the TCP client socket */
			shutdown(client_sock, SHUT_RDWR);
			close(client_sock);
		}
	}

	/* Gracefully close the TCP server socket */
	shutdown((size_t)server_sock, SHUT_RDWR);
	close((size_t)server_sock);

	return NULL;
}
/*}}}*/
