/* dhlist.c: a doubly connected list with head */
# include <stdlib.h>
# include <string.h>
# include "dhlist.h"

/* 
 * A node of the list contains some data,
 * and a link to the previous and next nodes.
 */
struct DhNode {
  void *data;
  dhlist next, previous;
};

int
dhlist_init (dhlist *head)
{ 
  /* initialize a doubly connected list with head */
  if ((*head = (dhlist) malloc (sizeof (struct DhNode))) == NULL) {
    /* no memory error */
    return 0;
  }
  /* the head's data section will be used as a length counter */
  (*head) -> data = (int) 0;
  (*head) -> next = NULL;
  (*head) -> previous = NULL;

  return 1;
}

int
dhlist_length (dhlist head)
{
  if (head == NULL) return 0;
  return (int) (head -> data);
}

dhlist
dhlist_first (dhlist head)
{ 
  /* return list's first item (head not included) */
  if (head == NULL) return NULL;
  if (head -> next == NULL) return head;
  return head -> next;
}

dhlist
dhlist_last (dhlist head)
{ 
  /* return list's last item (head not included) */
  if (head == NULL) return NULL;
  if (head -> previous == NULL) return head -> previous;
  return head -> previous;
}

dhlist
dhlist_end (dhlist head)
{ 
  /* stop iterating over a list when you reach its head */
  return head;
}

dhlist
dhlist_next (dhlist node)
{ 
  /* get the next item */
  if (node == NULL) return NULL;
  return node -> next;
}

void * 
dhlist_data (dhlist node)
{
  /* get the data of this node */
  if (node == NULL) return NULL;
  return node -> data;
}

int
dhlist_append (dhlist head, void *entry)
{
  /* insert an item in the list */
  dhlist dh;

  if (head == NULL) {
    /* invalid arguments */
    return 0;
  }
  if ((dh = (dhlist) malloc (sizeof (struct DhNode))) == NULL) {
    /* no memory */
    return 0;
  }
  dh -> data = entry;
  dh -> next = head;
  if (head -> previous != NULL) {
    dh -> previous = head -> previous;
    head -> previous -> next = dh;
  }
  else {
    dh -> previous = head;
    head -> next = dh;
  }
  head -> previous = dh;
  head -> data += (int) 1;
  return 1;
}

dhlist
dhlist_insert (dhlist head, dhlist pos, void *entry, char mode)
{
  /* 
   * insert an item in the list after or before (according to mode) 
   * a specified position 
   */
  dhlist dh;

  if (head == NULL) {
    /* invalid arguments */
    return NULL;
  }
  if (head == pos || pos == NULL) {
    if (!dhlist_append (head, entry))
      return NULL;
    else
      return dhlist_last (head);
  }
  if ((dh = (dhlist) malloc (sizeof (struct DhNode))) == NULL) {
    /* no memory */
    return NULL;
  }
  dh -> data = entry;
  switch (mode) {
  case AFTER:
    dh -> next = pos -> next;
    pos -> next = dh;
    dh -> previous = pos;
    if (dh -> next != NULL)
      dh -> next -> previous = dh;
    break;
  case BEFORE:
    dh -> previous = pos -> previous;
    pos -> previous = dh;
    dh -> next = pos;
    if (dh -> previous != NULL)
      dh -> previous -> next = dh;
    break;
  default:
    free (dh);
    return NULL;
  }
  head -> data += (int) 1;
  return dh;
}

void
dhlist_merge (dhlist dh1, dhlist dh2)
{ /* merge two lists */
  if (dh2 == NULL) return;
  if (dh1 == NULL) return;
  if (!(int) (dh2 -> data)) return;

  dh1 -> data += (int) (dh2 -> data);
  if (dh1 -> next == NULL) {
    dh1 -> next = dh2 -> next;
    dh2 -> next -> previous = dh1;
    dh1 -> previous = dh2 -> previous;
    dh2 -> previous -> next = dh1;
  }
  else {
    dh1 -> previous -> next = dh2 -> next;
    dh2 -> next -> previous = dh1 -> previous;
    dh1 -> previous = dh2 -> previous;
    dh1 -> previous -> next = dh1;
  }
  return;
}

static void
__del (dhlist node)
{ 
  /* delete a node. only for internal use. */

  if (node == NULL) return;
  
  if (node -> previous != NULL) {
    node -> previous -> next = node -> next;
    node -> next -> previous = node -> previous;
  }
  free (node);
  return;
}

int
dhlist_remove (dhlist head, dhlist node)
{ 
  /* remove the "node" item of a list */
  if (head == NULL) {
    /* invalid arguments */
    return 0;
  }
  if (node == NULL) return 1;
  if (head == node) {
    /* invalid arguments */
    return 0;
  }
  head -> data -= (int) 1;
  __del (node);
  return 1;
}

int
dhlist_delete (dhlist head)
{
  /* delete the whole list */
  dhlist tmp;

  if (head == NULL) {
    /* invalid arguments */
    return 0;
  }
  if (head -> previous == NULL) {
    free (head);
    return 1;
  }
  head -> previous -> next = NULL;
  tmp = head;
  while (tmp != NULL) {
    head = tmp;
    tmp = head -> next;
    free (head);
  }
  return 1;
}

int
dhlist_copy (dhlist *dest, dhlist src)
{
  dhlist cur;

  if (!dhlist_init (dest))
    return 0;

  for (cur = dhlist_first(src); cur != dhlist_end(src); cur = dhlist_next(cur))
    if (!dhlist_append (*dest, dhlist_data (cur))) {
      dhlist_delete (*dest);
      return 0;
    }
  return 1;
}

void
dhlist_modify (dhlist node, void *entry)
{
  if (node == NULL) return;
  node -> data = entry;
  return;
}

dhlist
dhlist_previous (dhlist node)
{
  if (node == NULL) return NULL;
  return node -> previous;
}

dhlist
dhlist_find (dhlist head, void * key, int (*compar) (void *, void *))
{
  /* find the list item which equals key according to compar routine */
  dhlist node;

  if (head == NULL || head -> data == (int) 0)
    return NULL;
  for (node = head -> next; node != head; node = node -> next)
    /* upon equality compar returns 0 */
    if (!compar (node -> data, key))
      return node;
  return NULL;
}

dhlist
dhlist_subset (dhlist head, int (* filter) (void *))
{
  /* create a list containing all items satisfying filter */
  dhlist node, nlist;

  if (head == NULL || head -> data == (int) 0)
    return NULL;
  if (!dhlist_init (&nlist))
    return NULL;

  for (node = head -> next; node != head; node = node -> next)
    if (filter (node -> data))
      if (!dhlist_append (nlist, node -> data)) {
        while ((int) (nlist -> data))
          dhlist_remove (nlist, nlist -> next);
        dhlist_delete (nlist);
        return NULL;
      }
  return nlist;
}

