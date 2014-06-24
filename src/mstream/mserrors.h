# ifndef __MUZIQ_STREAMER_ERRORS__
# define __MUZIQ_STREAMER_ERRORS__

int MS_errno;
int MS_pthread_errno;

void MSperror (char *);
void MShelp   (char *);

# define MSE_OK                  1

# define MSE_NOMEM              -1
# define MSE_OS                 -2
# define MSE_OPTIONAGAIN        -3
# define MSE_INVALIDPORTNUM     -5
# define MSE_INVALIDTHREADNUM   -8
# define MSE_READREQUEST       -13
# define MSE_BADREQUEST        -21
# define MSE_SOCKET            -34
# define MSE_BIND              -55
# define MSE_LISTEN            -89
# define MSE_PTHREAD          -144
# define MSE_ACCEPTCON        -233
# define MSE_WRITERESPONSE    -377
# define MSE_SIGNAL           -610
# define MSE_UNKNOWNOPTION    -987
# define MSE_SETSOCKOPT      -1597

# endif

