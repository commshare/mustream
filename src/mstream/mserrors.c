/* mserrors.c: error & help printing */
# include <stdio.h>
# include <string.h>
# include "mserrors.h"

void
MShelp (char *prog)
{
  fprintf (stderr, "usage: %s -p portnum -d musicdir [-t threadnum]\n", prog);
  return;
}

void
MSperror (char *errmsg)
{
  switch (MS_errno) {
  case MSE_NOMEM:
  case MSE_OS:
  case MSE_READREQUEST:
  case MSE_SOCKET:
  case MSE_BIND:
  case MSE_LISTEN:
  case MSE_ACCEPTCON:
  case MSE_WRITERESPONSE:
  case MSE_SIGNAL:
  case MSE_SETSOCKOPT:
    fprintf (stderr, "[--] ");
    perror (errmsg);
    break;
  case MSE_PTHREAD:
    fprintf (stderr, "[--] %s", strerror (MS_pthread_errno));
    break;
  case MSE_OPTIONAGAIN:
    fprintf (stderr, "[--] %s%sOption repetition.\n", 
	     errmsg == NULL ? "": errmsg, errmsg == NULL ? "": ": ");
    break;
  case MSE_INVALIDPORTNUM:
    fprintf (stderr, "[--] %s%sInvalid port specification.\n", 
	     errmsg == NULL ? "": errmsg, errmsg == NULL ? "": ": ");
    break;
  case MSE_INVALIDTHREADNUM:
    fprintf (stderr, "[--] %s%sInvalid threadpool specifier.\n", 
	     errmsg == NULL ? "": errmsg, errmsg == NULL ? "": ": ");
    break;
  case MSE_UNKNOWNOPTION:
    fprintf (stderr, "[--] %s%sUnknown option.\n", 
	     errmsg == NULL ? "": errmsg, errmsg == NULL ? "": ": ");
    break;
  case MSE_BADREQUEST:
    fprintf (stderr, "[--] %s%sBad request received.\n", 
	     errmsg == NULL ? "": errmsg, errmsg == NULL ? "": ": ");
    break;
  default:
    fprintf (stderr, "[--] %s%sSuccess.\n", 
	     errmsg == NULL ? "": errmsg, errmsg == NULL ? "": ": ");
    break;
  }

  return;
}
