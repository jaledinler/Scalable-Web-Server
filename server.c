#include "cs537.h"
#include "request.h"
#include <pthread.h>

// 
// server.c: A very, very simple web server
//
// To run:
//  server <portnum (above 2000)>
//
// Repeatedly handles HTTP requests sent to this port number.
// Most of the work is done within routines written in request.c
//

// CS537: Parse the new arguments too
pthread_cond_t empty; // = PTHREAD_COND_INITIALIZER;
pthread_cond_t fill;// = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex;// = PTHREAD_MUTEX_INITIALIZER;
int count = 0;
int fill_ptr = 0;
int use_ptr  = 0;
int *buffer;
int MAX;
void getargs(int *port, int *numThreads, int *bufferSize, int argc, char *argv[])
{
    if (argc != 4) {
  fprintf(stderr, "Usage: %s <port>\n", argv[0]);
  exit(1);
    }
    *port = atoi(argv[1]);
    *numThreads = atoi(argv[2]);
    *bufferSize = atoi(argv[3]);
}

void put(int fd) {
  buffer[fill_ptr] = fd;
  fill_ptr = (fill_ptr + 1) % MAX;
  count++;
  //printf("put___ fd: %d, count: %d\n",fd, count);
}
int get() {
  int fd = buffer[use_ptr];
  use_ptr = (use_ptr + 1) % MAX;
  count--;
  //printf("get___ fd: %d, count: %d\n",fd, count);
  return fd;
}
void *consume(void* arg) {
  while(1) {
    pthread_mutex_lock(&mutex);
    while (count == 0)
        pthread_cond_wait(&fill, &mutex);
    int fd = get();
    pthread_cond_signal(&empty);
    pthread_mutex_unlock(&mutex);
    requestHandle(fd);
    Close(fd);
  }
    return NULL;
}
void *produce(void *arg) {
  int *port = (int*) arg;
  int listenfd, connfd, clientlen;
  struct sockaddr_in clientaddr;
  listenfd = Open_listenfd(*port);
  while (1) {
      clientlen = sizeof(clientaddr);
      connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);
      pthread_mutex_lock(&mutex);
      while (count == MAX)
        pthread_cond_wait(&empty, &mutex);
      put(connfd);
      pthread_cond_signal(&fill);
      pthread_mutex_unlock(&mutex);
  } 
  Close(listenfd);
}
void create_consumers(int numThreads, pthread_t *ps) {
    int i;
    for (i = 0; i < numThreads; i++) {
        pthread_t p;
        pthread_create(&p, NULL, consume, NULL);
        ps[i] = p;
    }
}

pthread_t create_producer(int *port) {
    pthread_t p;
    pthread_create(&p, NULL, produce, port);
    return p;
}

void thread_join(int numThreads, pthread_t *consumers) {
  int i;
  for (i = 0; i < numThreads; i++) {
        pthread_join (consumers[i], NULL);
    }
}
int main(int argc, char *argv[])
{
    int port, numThreads, bufferSize;
    pthread_t *consumers;
    pthread_t producer;
    getargs(&port, &numThreads, &bufferSize, argc, argv); 
    MAX = bufferSize;
    buffer = malloc(MAX*sizeof(int));
    consumers = malloc(numThreads*sizeof(pthread_t));
    create_consumers(numThreads, consumers);
    producer = create_producer(&port);
    pthread_join (producer, NULL);
    thread_join(numThreads, consumers);
    free(consumers);
    free(buffer);
    exit(0);
}
