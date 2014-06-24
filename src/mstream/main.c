/* main.c: the main routine of mustream */
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <unistd.h>
# include <errno.h>
# include <limits.h>
# include <signal.h>

# include "mserrors.h"
# include "../sharedlib/dhlist.h"
# include "../playlist/playlist.h"
# include "../network/serve.h"

# define DEFAULT_THREAD_NUM 15

int    listenfd   = -1;   /* descriptor of the listening socket */
dhlist library    = NULL; /* music library */

/* server will stop when a SIGINT is received */
void
stop_serving (int signal)
{
  if (signal == SIGINT) {
    if (listenfd > -1) close (listenfd);
    fprintf (stdout, "Going down for maintenance!\n");
    exit (EXIT_SUCCESS);
  }
  return;
}

int main (int argc, char *argv[])
{
  char *musicdir = NULL, *endptr;
  int portid = 0, option, thread_num = -1;
  pthread_t *thread_pool;

  MS_errno = MSE_OK;
  MS_pthread_errno = 0;

  if (argc < 5 || argc > 7) {
    MShelp (argv [0]);
    exit (EXIT_FAILURE);
  }
  if (!dhlist_init (&library)) {
    MS_errno = MSE_NOMEM;
    MSperror ("Library initialisation failed");
    exit (EXIT_FAILURE);
  }

  /* read options */
  while ((option = getopt (argc, argv, "p:d:t:h")) != -1)
    switch (option) {
    case 'p': /* port option */
      if (portid) { /* if port option was re used */
        MS_errno = MSE_OPTIONAGAIN;
        MSperror ("Environment initialisation failed");
        if (musicdir != NULL) free (musicdir);
        dhlist_delete (library);
        exit (EXIT_FAILURE);
      }
      portid = strtol (optarg, &endptr, 10);
      if ((errno == ERANGE && (portid == LONG_MAX || portid == LONG_MIN)) 
          || (errno && !portid) || optarg == endptr || *endptr != '\0'
          || portid < 0) {
        MS_errno = MSE_INVALIDPORTNUM;
        MSperror ("Environment initialisation failed");
        if (musicdir != NULL) free (musicdir);
        dhlist_delete (library);
        exit (EXIT_FAILURE);
      }
      break;
    case 'd': /* music directory option */
      if (musicdir != NULL) {
        MS_errno = MSE_OPTIONAGAIN;
        MSperror ("Environment initialisation failed");
        if (musicdir != NULL) free (musicdir);
        dhlist_delete (library);
        exit (EXIT_FAILURE);
      }
      if ((musicdir = strdup (optarg)) == NULL) {
        MS_errno = MSE_NOMEM;
        MSperror ("Environment initialisation failed");
        dhlist_delete (library);
        exit (EXIT_FAILURE);
      }
      if (musicdir [strlen (musicdir) - 1] == '/')
        musicdir [strlen (musicdir) - 1] = '\0';
      break;
    case 't': /* threadpool option */
      thread_num = strtol (optarg, &endptr, 10);
      if (thread_num < 1 
          || (errno == ERANGE 
              && (thread_num == LONG_MAX || thread_num == LONG_MIN)) 
          || (errno && !thread_num) || optarg == endptr || *endptr != '\0') {
        MS_errno = MSE_INVALIDTHREADNUM;
        MSperror ("Environment initialisation failed");
        if (musicdir != NULL) free (musicdir);
        dhlist_delete (library);
        exit (EXIT_FAILURE);
      }
      break;
    case 'h': /* help option */
      MShelp (argv [0]);
      exit (EXIT_SUCCESS);
    default:
      if (musicdir != NULL) free (musicdir);
      dhlist_delete (library);
      MS_errno = MSE_UNKNOWNOPTION;
      exit (EXIT_FAILURE);
    }

  /* initialise music library */
  if (build_library (musicdir, library) != MSE_OK) {
    MSperror ("Unable to build music library");
    free (musicdir);
    dhlist_delete (library);
    exit (EXIT_FAILURE);
  }
  free (musicdir);

  /* handle signals */
  if (signal (SIGPIPE, SIG_IGN) == SIG_ERR
      || signal (SIGINT, stop_serving) == SIG_ERR) {
    MS_errno = MSE_SIGNAL;
    MSperror ("Unable to initialise environment");
    dhlist_delete (library);
    exit (EXIT_FAILURE);
  }
  /* start listening to the specified port */
  if ((listenfd = network_init (portid)) < 0) {
    MSperror ("Unable to get online");
    dhlist_delete (library);
    exit (EXIT_FAILURE);
  }
  
  /* create the threapool that will serve any clients */
  if (thread_num < 0) thread_num = DEFAULT_THREAD_NUM;
  if (create_threadpool (&thread_pool, thread_num) != MSE_OK) {
    MSperror ("Unable to receive incoming connections");
    close (listenfd);
    dhlist_delete (library);
    exit (EXIT_FAILURE);
  }

  /* job's done */
  for (; ;)
    pause ();
}
