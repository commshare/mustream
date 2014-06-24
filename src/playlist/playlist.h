# ifndef __PLAYLIST_HANDLING_LIB__
# define __PLAYLIST_HANDLING_LIB__

# include "../sharedlib/dhlist.h"

int build_library (char *, dhlist);
int search_library (dhlist, dhlist *, char *);

# endif
