/* strmod.c: string modification routines */
# include <stdio.h>
# include <string.h>
# include <stdlib.h>
# include <stdarg.h>
# include <ctype.h>

static char *
detach_word (char * input, char delim, int *length)
{ /* detach characters from input until a delim is met. */
  int count = 0, ignore = 0;
  char * retc;

  /* ignore starting delim characters */
  *length = 0;
  while (input [count] == delim) {
    (*length) ++;
    input ++;
  }
  /* do not check any quoted phrase for the delim */
  if (input [count] == '\"') {
    ignore = 1;
    (*length) ++;
    input ++;
  }
  while (input [count] != '\0' && (input [count] != delim || ignore)
	 && (input [count] != '\"' || !ignore))
    count ++;
    
  if (!count) return (char *) -1;

  if ((retc = (char *) malloc ((count + 2) * sizeof (char))) == NULL)
    return NULL;
  
  *length += count;
  retc [count] = retc [count + 1] = '\0';
  for (-- count; count > - 1; count --)
    retc [count] = input [count];

  return retc;
}

char *
parse_string (char ** input, char delim)
{ /* parse a string and set input in the next available position */
  char * joker;
  int length;

  if (*input == '\0')
    return (char *) -1;

  if ((joker = detach_word (*input, delim, &length)) == NULL)
    return NULL;
  else if (joker == (char *) -1)
    return (char *) -1;
  *input += length + 1;
  return joker;
}

int
split_delim (char ***table, char *input, char delimiter)
{ /* split input into parts divided by the delimiter */
  int i, count = 0, flag = 0, length;

  /* decide the length of the table */
  for (i = 0; i < strlen (input); i ++)
    if (input [i] == delimiter) {
      flag = 1;
      count ++;
    }
    else if (input [i] == '\"') { /* ignore quoted phrases */
      flag = 0;
      do {
	i ++;
      } while (input [i] != '\"' && i < strlen (input));
    }
    else if (!isspace (input [i])) flag = 0;
  /* if flag is 0 then the final delimiter was not in the end */
  
  if (!count) { /* if no delimiter was found */
    if ((*table = (char **) calloc (1, sizeof (char *))) == NULL)
      return 0;
    if ((**table = calloc (strlen (input) + 2, sizeof (char))) == NULL) {
      free (*table);
      return 0;
    }
    strcpy (**table, input);
    return 1;
  }

  if (!flag) count ++;

  if ((*table = (char **) calloc (count, sizeof (char *))) == NULL)
    return 0;

  for (i = 0; i < count; i ++) {
    if (((*table) [i] = detach_word (input, delimiter, &length)) == NULL) {
      for (i = 0; i < count; i ++)
	if ((*table) [i] != NULL) 
	  free ((*table) [i]);
      free (*table);
    }
    input += length + 1;
  }
  
  return count;
}

char *
cutup_string (char ** input, char delim)
{
  char **commands, *res;
  int length, i;

  if (!(length = split_delim (&commands, *input, delim)))
    return NULL;
  for (i = 1; i < length; i ++) 
    if (commands [i] != NULL) free (commands [i]);
  res = commands [0];
  free (commands);
  *input += strlen (res) + 1;
  return res;
}

char *
Sprintf (char *fmt, ...)
{ /* allocate a string and print the printf-like formatted input */
  va_list ap;
  char *cursor;
  int size;

  va_start (ap, fmt);
  if ((size = vsnprintf (cursor, 0, fmt, ap)) < 0)
    return NULL;
  va_end (ap);

  if ((cursor = (char *) calloc (size + 1, sizeof (char))) == NULL)
    return NULL;

  va_start (ap, fmt);
  if (vsnprintf (cursor, size + 1, fmt, ap) < 0) {
    free (cursor);
    return NULL;
  }
  va_end (ap);

  return cursor;
}

char *
Vsprintf (char *fmt, va_list arguments)
{ /* same as Sprintf, but takes a va_list argument */
  va_list ap = arguments;
  char *cursor;
  int size;

  if ((size = vsnprintf (cursor, 0, fmt, ap)) < 0)
    return NULL;

  if ((cursor = (char *) calloc (size + 1, sizeof (char))) == NULL)
    return NULL;

  if (vsnprintf (cursor, size + 1, fmt, arguments) < 0) {
    free (cursor);
    return NULL;
  }

  return cursor;
}

char *
strcut (char *haystack, char *needle)
{ /* cut needle postfix from haystack, if it exists */
  int i, j, haystack_len, needle_len;
  char *result;

  haystack_len = strlen (haystack) - 1;
  needle_len = strlen (needle) - 1;

  if (haystack_len < needle_len)
    return (char *) -1;

  for (i = haystack_len, j = needle_len; j > -1; i--, j--)
    if (haystack [i] != needle [j])
      return (char *) -1;

  if ((result = calloc (i+2, sizeof (char))) == NULL)
    return NULL;

  memcpy (result, haystack, i+1);

  return result;
}

/***************/

typedef struct stringList * str; 

struct stringList {
  char ch;
  str next;
};


static void 
freeList (str start)
{
  str tmp;
  
  while (start != NULL) {
    tmp = start;
    start = start -> next;
    free (tmp);
  }
}

int 
fread_delim (char ** inpString, FILE * inpFile, char delimiter)
{
  /*
   * reads characters from the input file until it reaches the specified 
   * delimiter or EOF and saves them in a linked list. then, creates a char 
   * array of sufficient space, and copies there the string that is stored in
   * the list. finally, it frees the list and returns the string via its 
   * argument. returns 0 upon successful completion, 1 otherwise.
   */

  char c;
  str l, start;
  short first_time = 1;
  int counter = 0, i = 0;
  
  while ((c = fgetc (inpFile)) != EOF && c != delimiter) {
    if (first_time) { /* if this is the first character that you read, */
      if ((l = (str) malloc (sizeof (struct stringList))) == NULL)
	return 0;
      l -> ch = c;
      l -> next = NULL;
      start = l;
      first_time = 0;
    }
    else {
      if ((l -> next = (str) malloc (sizeof (struct stringList))) == NULL) {
	freeList (start);
	return 0;
      }
      l -> next -> ch = c;
      l -> next -> next = NULL;
      l = l -> next;
    }
    counter ++;
  }
  if (!counter) { /* if the first character read was EOF or the delimiter */
    * inpString = NULL;
    return 1;
  }
  /* create the string */
  if ((* inpString = (char *) malloc ((counter + 2) * sizeof (char))) == NULL) {
    freeList (start);
    return 0;
  }
  for (i = 0, l = start; i < counter; i ++, l = l -> next)
    (* inpString) [i] = l -> ch;
  /* the created string must be null-terminated */
  (* inpString) [counter] = '\0';
  /* to make possible an abstracted string-handler */
  (* inpString) [counter + 1] = '\0';
  
  freeList (start);
  return 1;
}

