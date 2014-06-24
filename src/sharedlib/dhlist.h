# ifndef __DOUBLY_CONNECTED_LIST_WITH_HEADER__
# define __DOUBLY_CONNECTED_LIST_WITH_HEADER__

# define BEFORE 0
# define AFTER  1

typedef struct DhNode * dhlist;

int    dhlist_init     (dhlist *);
int    dhlist_length   (dhlist);
dhlist dhlist_first    (dhlist);
dhlist dhlist_last     (dhlist);
dhlist dhlist_end      (dhlist);
dhlist dhlist_next     (dhlist);
dhlist dhlist_previous (dhlist);
void * dhlist_data     (dhlist);
int    dhlist_append   (dhlist, void *);
dhlist dhlist_insert   (dhlist, dhlist, void *, char);
void   dhlist_merge    (dhlist, dhlist);
int    dhlist_remove   (dhlist, dhlist);
int    dhlist_delete   (dhlist);
void   dhlist_modify   (dhlist, void *);
int    dhlist_copy     (dhlist *, dhlist);
dhlist dhlist_find     (dhlist, void *, int (*compar) (void *, void *));
dhlist dhlist_subset   (dhlist, int (*filter) (void *));

# endif
