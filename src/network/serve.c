/* serve.c: actual network handlers */
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <unistd.h>
# include <sys/types.h>
# include <sys/socket.h>
# include <netdb.h>
# include <netinet/in.h>
# include <pthread.h>

# include "../mstream/mserrors.h"
# include "http.h"

# define LISTEN_BACKLOG 20

extern int listenfd; /* the listening socket descriptor */

  /* client address length */
int addrlen = sizeof (struct sockaddr_in);
  /* a mutex used to lock acceptance of connections between threads */
pthread_mutex_t mlock = PTHREAD_MUTEX_INITIALIZER;

/*
 * open up port #portid and start listening to it for incoming connections.
 * return a socket descriptor if succesfull, an error code otherwise.
 */
int
network_init (int portid)
{
  int                sockfd, reuse = 1;
  struct sockaddr_in servaddr;

  /* create the socket */
  if ((sockfd = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
    return (MS_errno = MSE_SOCKET);
  }
  /* set the reuse address flag */
  if (setsockopt (sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) < 0){
    close (sockfd);
    return (MS_errno = MSE_SETSOCKOPT);
  }

  /* prepare binding */
  memset (&servaddr, '\0', sizeof(struct sockaddr_in));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl (INADDR_ANY);
  servaddr.sin_port = htons (portid);

  if (bind 
       (sockfd, (struct sockaddr*) &servaddr, sizeof(struct sockaddr_in)) < 0){
    close (sockfd);
    return (MS_errno = MSE_BIND);
  }

  if (listen (sockfd, LISTEN_BACKLOG) < 0) {
    close (sockfd);
    return (MS_errno = MSE_LISTEN);
  }

  return sockfd;
}

/* resolve client's name */
static char *
__client_id (struct sockaddr *cliaddr, socklen_t clilen)
{
  char *peername;
  int   peerlen = 40, val;

  do {
    if ((peername = (char *) calloc (peerlen, sizeof (char))) == NULL)
      return NULL;
    val = getnameinfo (cliaddr, clilen, peername, peerlen, NULL, 0, 0);
    if (!val) break; /* everything ok, return */
    if (val == EAI_OVERFLOW) { /* while peername is not enough */
      peerlen += 25;
      free (peername);
    }
    else { /* error occured */
      free (peername);
      return NULL;
    }
  } while (1);

  return peername;
}

/*
 * this is the function executed by the threads of the pool.
 * just accepts connections and fullfills requests.
 */
void *
serve_client (void *arg)
{
  struct sockaddr *cliaddr;
  socklen_t        clilen;
  int              connfd;
  char            *peername;
  HTTPRequest      request;
  HTTPResponse     response;

  if ((cliaddr = (struct sockaddr *) malloc (addrlen)) == NULL)
    pthread_exit ((void*) MSE_NOMEM);
  memset (cliaddr, '\0', addrlen);

  while (1) {
    /* initialise structures */
    request = NULL;
    response = NULL;
    clilen = addrlen;
    memset (cliaddr, '\0', addrlen);

    pthread_mutex_lock (&mlock); /* lock connection acceptance */
    connfd = accept (listenfd, cliaddr, &clilen);
    pthread_mutex_unlock (&mlock); /* unlock it */

    /* if an error happened report it and go on */
    if (connfd < 0) { 
      MS_errno = MSE_ACCEPTCON;
      MSperror ("Unable to accept pending connection");
      continue;
    }

    /* specify peer name */
    if ((peername = __client_id (cliaddr, clilen)) == NULL) {
      MS_errno = MSE_NOMEM;
      MSperror ("Cannot resolve peer name");
      goto LoopEpilogue;      
    }
    /* read client's request */
    if (read_request (connfd, &request) != MSE_OK) {
      MSperror (peername);
      /* create a 500 server error response */
      if (form_response (NULL, &response) != MSE_OK) {
        goto LoopEpilogue;
      }
    }
    else { /* create the response in a normal way */
      print_request (peername, request);
      if (form_response (request, &response) != MSE_OK) {
        MSperror (peername);
        goto LoopEpilogue;
      }
    }

    /* send the response to the client */
    if (write_response (connfd, response) != MSE_OK) {
      MSperror (peername);
      goto LoopEpilogue;
    }
    print_response (peername, response);

   LoopEpilogue:
    close (connfd);
    transaction_done (request, response);
    if (peername != NULL) free (peername);
  }
}

/*
 * creates a pool of threads for serving clients concurrently. incoming
 * connections are handled only by these threads. a mutex lock secures
 * the accept system call, so a connection will be served by a unique
 * thread. 
 * the following routine will create a pool of thread_tnum threads and
 * will save their ids in the thread_tids table.
 */
int
create_threadpool (pthread_t **thread_tids, int thread_tnum)
{
  int i;

  *thread_tids = (pthread_t *) calloc (thread_tnum, sizeof (pthread_t));
  if (*thread_tids == NULL)
    return (MS_errno = MSE_NOMEM);

  for (i = 0; i < thread_tnum; i ++)
    /* 
     * each thread will start on executing the serve_client module 
     * (no need for any arguments)
     */
    if (MS_pthread_errno =
          pthread_create (&((*thread_tids)[i]), NULL, &serve_client, NULL)) {
      free (*thread_tids);
      return (MS_errno = MSE_PTHREAD);
    }

  return MSE_OK;
}

