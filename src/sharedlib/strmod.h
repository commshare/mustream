# ifndef __STRING_MODIFICATION_LIB__
# define __STRING_MODIFICATION_LIB__

# include <stdarg.h>
# include <stdio.h>

char* parse_string (char**, char);
char* cutup_string (char**, char);
int   split_delim  (char***, char*, char);
char* Sprintf      (char*, ...);
char* Vsprintf     (char*, va_list);
char* strcut       (char*, char*);
int   fread_delim  (char**, FILE*, char);

# endif

