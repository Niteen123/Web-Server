/* run using ./server <port> */
#include "http_server.hh"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <pthread.h>
#define threadcount 4
#define queuesize 100
char buffer[1024];

pthread_cond_t condQueue;
pthread_mutex_t mutexQueue;
pthread_cond_t condQueueFull;
int requests;
int shared_buffer[queuesize];

void *serve(void *fd);
void error(char *msg)
{
  perror(msg);
  exit(1);
}

void *thread_handler(void *n)
{

  while (1)
  {
    int i,client;
    pthread_mutex_lock(&mutexQueue);
    while (requests == 0)
    {
      pthread_cond_wait(&condQueue, &mutexQueue);
    }

    client = shared_buffer[0];
    
    for (i = 0; i < requests - 1; i++)
    {
      shared_buffer[i] = shared_buffer[i + 1];
    }
    requests--;
    pthread_cond_signal(&condQueueFull);
    pthread_mutex_unlock(&mutexQueue);
    serve(&client);
    close(client);
  }
}
void *exec_work(int socket)
{
  pthread_mutex_lock(&mutexQueue);
  shared_buffer[requests++] = socket;
  pthread_cond_signal(&condQueue);
  pthread_mutex_unlock(&mutexQueue);
  return NULL;
}

void *serve(void *fd)
{
  int n;
  int sockfd = *((int *)fd);
  while (1)
  {
    bzero(buffer, 1024);
    n = read(sockfd, buffer, 1023);
    // printf("Here is the message: %s", buffer);
    if (n == 0)
      return NULL;
    /* send reply to client */
    HTTP_Response *response = handle_request(buffer);
    string response_string = response->get_string();
    n = write(sockfd, response_string.c_str(), response_string.size());
    if (n < 0)
      error("ERROR writing to socket");
    bzero(buffer, 1024);
  }
  close(sockfd);
}

int main(int argc, char *argv[])
{
  int sockfd, newsockfd, portno;
  socklen_t clilen;

  struct sockaddr_in serv_addr, cli_addr;
  int n;

  if (argc < 2)
  {
    fprintf(stderr, "ERROR, no port provided\n");
    exit(1);
  }

  /* create socket */

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0)
    error("ERROR opening socket");

  /* fill in port number to listen on. IP address can be anything (INADDR_ANY)
   */

  bzero((char *)&serv_addr, sizeof(serv_addr));
  portno = atoi(argv[1]);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);

  /* bind socket to this port number on this machine */

  if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    error("ERROR on binding");

  /* listen for incoming connection requests */

  listen(sockfd, 5);
  clilen = sizeof(cli_addr);

  pthread_t threads[threadcount];
  for (int i = 0; i < threadcount; i++)
  {
    n = pthread_create(&threads[i], NULL, thread_handler, &i);
  }
  if (n)
  {
    cout << "Error:unable to create thread," << n << endl;
    exit(1);
  }

  /* accept a new request, create a newsockfd */
  while (1)
  {
    pthread_mutex_lock(&mutexQueue);
    if (requests == queuesize)
    {
      pthread_cond_wait(&condQueueFull, &mutexQueue);
    }
    pthread_mutex_unlock(&mutexQueue);

    newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
    if (newsockfd < 0)
      error("ERROR on accept");
    exec_work(newsockfd);
    /* read message from client */
    /* else
     {
       pthread_t thread_id;
       pthread_create(&thread_id, NULL, serve, &newsockfd);
     }*/
  }
  /*pthread_exit(NULL);
  for (int i = 0; i < threadcount; i++)
  {
    if (pthread_join(threads[i], NULL) != 0)
    {
      perror("Failed to join the thread");
    }
  }*/
  pthread_mutex_destroy(&mutexQueue);
  pthread_cond_destroy(&condQueueFull);
  pthread_cond_destroy(&condQueue);
  close(sockfd);
  return 0;
}
