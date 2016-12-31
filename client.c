/*
 * client.c: A very, very primitive HTTP client.
 * 
 * To run, try: 
 *      client www.cs.wisc.edu 80 /
 *
 * Sends one HTTP request to the specified HTTP server.
 * Prints out the HTTP response.
 *
 * CS537: For testing your server, you will want to modify this client.  
 * For example:
 * 
 * You may want to make this multi-threaded so that you can 
 * send many requests simultaneously to the server.
 *
 * You may also want to be able to request different URIs; 
 * you may want to get more URIs from the command line 
 * or read the list from a file. 
 *
 * When we test your server, we will be using modifications to this client.
 *
 */

#include "cs537.h"
#include <pthread.h>

char *host;
int port;
char** lines;
int count = 0; 
/*
 * Read the HTTP response and print it out
 */
void clientPrint(int fd)
{
  rio_t rio;
  char buf[MAXBUF];  
  int length = 0;
  int n;
  
  Rio_readinitb(&rio, fd);

  /* Read and display the HTTP Header */
  n = Rio_readlineb(&rio, buf, MAXBUF);
  while (strcmp(buf, "\r\n") && (n > 0)) {
    printf("Header: %s", buf);
    n = Rio_readlineb(&rio, buf, MAXBUF);

    /* If you want to look for certain HTTP tags... */
    if (sscanf(buf, "Content-Length: %d ", &length) == 1) {
      printf("Length = %d\n", length);
    }
  }

  /* Read and display the HTTP Body */
  n = Rio_readlineb(&rio, buf, MAXBUF);
  while (n > 0) {
    printf("%s", buf);
    n = Rio_readlineb(&rio, buf, MAXBUF);
  }
}

/*
 * Send an HTTP request for the specified file 
 */
void clientSend(int fd, char *filename)
{
  char buf[MAXLINE];
  char hostname[MAXLINE];

  Gethostname(hostname, MAXLINE);

  /* Form and send the HTTP request */
  sprintf(buf, "GET %s HTTP/1.1\n", filename);
  sprintf(buf, "%shost: %s\n\r\n", buf, hostname);
  Rio_writen(fd, buf, strlen(buf));
}

void *execute(void *args) {
  int i;
  
  for (i = 0; i < count; i++) {
    int clientfd = Open_clientfd(host, port);
    clientSend(clientfd, lines[i]);
    clientPrint(clientfd);
    Close(clientfd);
  }
  
  
  //free(lines);
  return NULL;
}

void create_clients(int numClients, pthread_t *ps) {
    int i;
    for (i = 0; i < numClients; i++) {
        pthread_t p;
        pthread_create(&p, NULL, execute, NULL);
        ps[i] = p;
    }
}

void thread_join(int numClients, pthread_t *clients) {
  int i;
  for (i = 0; i < numClients; i++) {
        pthread_join (clients[i], NULL);
    }
}

int countLines(char *filename) {
  char *line;
  int count = 0;
  FILE *fp = NULL;
  if ((fp = fopen(filename, "r")) == NULL) {
        printf("Error! opening file");
        exit(1);         
  }
  line = malloc(100*sizeof(char));
  while (!feof(fp)) {
    fscanf(fp,"%s\n",line);
    count++;
  }
  free(line);
  fclose(fp);
  return count;
}

void readFile(char *filename) {
  count = countLines(filename);
  //char *lines[count];
  FILE *fp = NULL;
  if ((fp = fopen(filename, "r")) == NULL) {
        printf("Error! opening file");
        exit(1);         
  }
  int i = 0;
  lines = malloc(count*sizeof(char*));
  while (!feof(fp)) {
    lines[i] = malloc(100*sizeof(char));
    fscanf(fp,"%s\n",lines[i]);
    printf("%s\n", lines[i]);
    i++;
  }
  fclose(fp);
}

int main(int argc, char *argv[])
{
  int numClients;
  pthread_t *clients;

  if (argc != 5) {
    fprintf(stderr, "Usage: %s <host> <port> <filename> <number of clients>\n", argv[0]);
    exit(1);
  }

  host = argv[1];
  port = atoi(argv[2]);
  char *filename = argv[3];
  numClients = atoi(argv[4]);  
  readFile(filename);
  clients = malloc(numClients*sizeof(pthread_t));
  create_clients(numClients, clients);
  thread_join(numClients, clients);
  free(clients);
  int i;
  for (i = 0; i < count; i++) {
    free(lines[i]);
  }
  free(lines);
  exit(0);
}
