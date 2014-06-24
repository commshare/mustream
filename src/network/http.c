/* http.c: request handlers based on http */
# include <stdio.h>
# include <string.h>
# include <stdlib.h>
# include <unistd.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <fcntl.h>

# include "../sharedlib/dhlist.h"
# include "../sharedlib/strmod.h"
# include "../sharedlib/url_codec.h"
# include "../playlist/playlist.h"
# include "../playlist/spack.h"
# include "../mstream/mserrors.h"
# include "http.h"

# define BUFFERSIZE 512

# define __REQUESTED_SONG__     1
# define __REQUESTED_PLAYLIST__ 2

typedef enum {RESPONSE_FD = 0, RESPONSE_PL, RESPONSE_NO} restype;

extern dhlist library;

struct HTTP_Request {
  char   *command,  /* the command of the request (eg GET, etc) */
         *resource, /* the resource requested */
         *version;  /* HTTP version used */
  dhlist  headers;  /* request headers */
};

struct HTTP_Response {
  char    *version,       /* HTTP version used */
          *response_code; /* the response code (eg 200 OK, etc) */
  dhlist   headers;       /* response headers */
  void    *body;          /* body of the response (song, playlist) */
  restype  type;          /* body type: playlist, song or nothing */
  int      length;        /* if body is a playlist specify its length */
};


  /* set/unset if previous segment of request ended in CRLF or not */
static short int previous_crlf = 0;

/* check if two consecutive CRLF exist in buffer */
static int
__request_ends (char *buffer, int buflen)
{
  int i, cr = 0, crlf = previous_crlf;

  for (i = 0; i < buflen && crlf != 2; i ++)
    switch (buffer [i]) {
    case '\r':
      if (!cr) cr = 1;
      break;
    case '\n':
      if (cr) {
        crlf ++;
        cr = 0;
      }
      break;
    default:
      cr = 0;
      crlf = 0;
      break;
    }
  if (crlf == 1) previous_crlf = 1;
  else previous_crlf = 0;

  return crlf == 2;
}

/*
 * given a string containing an http request, 
 * format it to an HTTP_Request struct.
 */
static int
__request_format (HTTPRequest *request, char *strreq, int reqlen)
{
  char *cursor = strreq, *head;
 
  if ((*request = (HTTPRequest) malloc (sizeof (struct HTTP_Request))) == NULL)
    return (MS_errno = MSE_NOMEM);

  if (!dhlist_init (&(*request) -> headers)) {
    free (*request);
    return (MS_errno = MSE_NOMEM);
  }
  if (((*request) -> command = parse_string (&cursor, ' ')) == NULL) {
    dhlist_delete ((*request) -> headers);
    free (*request);
    return (MS_errno = MSE_NOMEM);
  }

  if (((*request) -> resource = parse_string (&cursor, ' ')) == NULL) {
    free ((*request) -> command);
    dhlist_delete ((*request) -> headers);
    free (*request);
    return (MS_errno = MSE_NOMEM);
  }

  if (((*request) -> version = parse_string (&cursor, '\r')) == NULL) {
    free ((*request) -> resource);
    free ((*request) -> command);
    dhlist_delete ((*request) -> headers);
    free (*request);
    return (MS_errno = MSE_NOMEM);
  }
  cursor ++;
  while ((head = parse_string (&cursor, '\r')) != (char *) -1) {
    if (!strcmp (head, "\n")) continue;
    if (head == NULL
        || !dhlist_append ((*request) -> headers, head)) {
      free ((*request) -> version);
      free ((*request) -> resource);
      free ((*request) -> command);
      dhlist_delete ((*request) -> headers);
      free (*request);
      return (MS_errno = MSE_NOMEM);
    }
    cursor ++;
  }

  return MSE_OK;
}

/* read a request from an accepted connection */
int
read_request (int connfd, HTTPRequest *request)
{
  char buffer [BUFFERSIZE], *total, *old = NULL;
  size_t bytes_read, total_bytes = 0;

  memset (buffer, '\0', BUFFERSIZE);

  while ((bytes_read = read (connfd, buffer, BUFFERSIZE)) >= 0) {
    total_bytes += bytes_read;
    if ((total = realloc (old, total_bytes+1)) == NULL) {
      if (old != NULL) free (old);
      return (MS_errno = MSE_NOMEM);
    }
    memcpy (total + total_bytes - bytes_read, buffer, bytes_read);
    if (__request_ends (buffer, bytes_read)) {
      total [total_bytes] = '\0';
      break;
    }
    old = total;
    memset (buffer, '\0', BUFFERSIZE);
  }

  if (bytes_read < 0) {
    free (total);
    return (MS_errno = MSE_READREQUEST);
  }
  if (__request_format (request, total, total_bytes) != MSE_OK) {
    free (total);
    return MS_errno;
  }

  free (total);
  return MSE_OK;
}

/* print a request informative message to stdout */
void
print_request (char *source, HTTPRequest request)
{
  char *resource_decoded, *to_print;
  int   reslen = strlen (request -> resource);

  if ((resource_decoded = (char *) calloc (reslen+1, sizeof(char))) == NULL
      || url_decode (request -> resource, resource_decoded, reslen, 1) < 0)
    to_print = request -> resource;
  else to_print = resource_decoded;

  fprintf (stdout, "[<-] %s requested %s under %s.\n", 
                   source, to_print, request -> version);

  if (to_print != request -> resource) free (to_print);  
  return;
}


/* given a response code and a content type initialise an HTTP_Response */
static int
__response_init (HTTPResponse *response, char *rcode, char *content_type)
{
  char *head;

  if ((*response = (HTTPResponse)malloc(sizeof(struct HTTP_Response))) == NULL)
    return (MS_errno = MSE_NOMEM);

  memset ((*response), '\0', sizeof (struct HTTP_Response));
  (*response) -> type = RESPONSE_NO;
  if (((*response) -> version = strdup ("HTTP/1.1")) == NULL) {
    free (*response);
    return (MS_errno = MSE_NOMEM);
  }
  if (((*response) -> response_code = strdup (rcode)) == NULL) {
    free ((*response) -> version);
    free (*response);
    return (MS_errno = MSE_NOMEM);
  }
  if (!dhlist_init (&((*response) -> headers))) {
    free ((*response) -> version);
    free ((*response) -> response_code);
    free (*response);
    return (MS_errno = MSE_NOMEM);
  }
  if ((head = strdup ("Server: muZiqStreamer v0.9")) == NULL
      || !dhlist_append ((*response) -> headers, head)) {
    free ((*response) -> version);
    free ((*response) -> response_code);
    dhlist_delete ((*response) -> headers);
    free (*response);
    return (MS_errno = MSE_NOMEM);
  }
  if ((head = strdup ("Connection: close")) == NULL
      || !dhlist_append ((*response) -> headers, head)) {
    free ((*response) -> version);
    free ((*response) -> response_code);
    dhlist_delete ((*response) -> headers);
    free (*response);
    return (MS_errno = MSE_NOMEM);
  }
  if (content_type != NULL 
      && ((head = Sprintf ("Content-Type: %s", content_type)) == NULL
          || !dhlist_append ((*response) -> headers, head))) {
    free ((*response) -> version);
    free ((*response) -> response_code);
    dhlist_delete ((*response) -> headers);
    free (*response);
    return (MS_errno = MSE_NOMEM);
  }     

  return MSE_OK;
}

/* decide if client requested a song or a playlist */
static int
__request_search (char *resource, char **song, char **search)
{
  char *str;

  if ((str = strstr (resource, "/songsearch/")) == NULL
      || str != resource) {
    if ((*song = strdup (resource)) == NULL)
      return (MS_errno = MSE_NOMEM);
    return __REQUESTED_SONG__;
  }
  str = resource + strlen ("/songsearch/");

  if (*str == '\0')
    *search = NULL;
  else if ((*search = strcut (str, ".m3u")) == NULL)
    return (MS_errno = MSE_NOMEM);
  if (*search == (char *) -1)
    return (MS_errno = MSE_BADREQUEST);

  return __REQUESTED_PLAYLIST__;
}

/* search through headers to find the 'Host:' one */
static char *
__get_host (dhlist headers)
{
  char *head, *str;
  dhlist cur;

  for (cur = dhlist_first (headers); cur != dhlist_end (headers);
       cur = dhlist_next (cur)) {
    head = (char *) dhlist_data (cur);
    if ((str = strstr (head, "Host:")) == NULL || str != head)
      continue;
    head += strlen ("Host:");
    if ((str = parse_string (&head, ' ')) == NULL) {
      MS_errno = MSE_NOMEM;
      return NULL;
    }
    return str;
  }
  return NULL;
}

/* given an HTTP request form the appropriate HTTP response */
int
form_response (HTTPRequest request, HTTPResponse *response)
{
  char *search, *song, *host;
  dhlist res, cur;
  spack songinfo;
  int i;

  if (request == NULL) { /* if an error occured while processing request */
    if (__response_init (response, "500 server error", NULL) != MSE_OK)
      return MS_errno;
    return MSE_OK;
  }
  if (strcmp (request -> command, "GET")) { /* only GET is supported */
    if (__response_init (response, "501 not implemented", NULL) != MSE_OK)
      goto ServerError;
    return MSE_OK;
  }
  /* http 1.0 & 1.1 currently supported */
  if ((strcmp (request -> version, "HTTP/1.1") 
       && strcmp (request -> version, "HTTP/1.0"))
      || (host = __get_host (request -> headers)) == NULL) {
    if (__response_init (response, "400 bad request", NULL) != MSE_OK)
      goto ServerError;
    return MSE_OK;
  }
  
  switch (__request_search (request -> resource, &song, &search)) {
  case __REQUESTED_SONG__: /* if client requested a song */
    /* find it in the library */
    if ((res = dhlist_find (library, song, &spack_filter)) == NULL) {
      if (__response_init (response, "404 not found", NULL) != MSE_OK)
        goto ServerError;
      return MSE_OK;
    }
    songinfo = (spack) dhlist_data (res);
    /* inform client about song content */
    if (__response_init(response, "200 OK", spack_content(songinfo)) != MSE_OK)
      goto ServerError;
    /* open the song file to send the actual song data */
    if (((*response) -> body = malloc (sizeof(int))) == NULL) {
      MS_errno = MSE_NOMEM;
      goto ServerError;
    }
    * (int*) ((*response) -> body) = 
               open (spack_server_path (songinfo), O_RDONLY);
    if (* (int*) ((*response) -> body) < 0) {
      MS_errno = MSE_OS;
      goto ServerError;
    }
    (*response) -> type = RESPONSE_FD;
    return MSE_OK;

  case __REQUESTED_PLAYLIST__: /* if client requested a playlist */
    /* search the library for the given string */
    if (search_library (library, &res, search) != MSE_OK)
      goto ServerError;
    if (!dhlist_length (res)) { /* if no matches were found */
      dhlist_delete (res);
      if (__response_init (response, "404 not found", NULL) != MSE_OK)
        goto ServerError;
      return MSE_OK;
    }
    /* initialise response */
    if (__response_init (response, "200 OK", "audio/x-mpegurl") != MSE_OK)
      goto ServerError;
    (*response) -> length = dhlist_length (res);
    /* and create a string array (will be sent as the message body) */
    (*response) -> body = calloc (dhlist_length (res), sizeof (char*));
    if ((*response) -> body == NULL) {
      MS_errno = MSE_NOMEM;
      dhlist_delete (res);
      goto ServerError;
    }
    for (i = 0, cur = dhlist_first (res); cur != dhlist_end (res); 
         cur = dhlist_next (cur), i ++) {
      songinfo = (spack) dhlist_data (cur);
      ((char **) ((*response) -> body)) [i] = spack_formal (songinfo, host);
      if (((char **) ((*response) -> body)) [i] == NULL) {
        MS_errno = MSE_NOMEM;
        dhlist_delete (res);
        goto ServerError;
      }
    }
    dhlist_delete (res);
    (*response) -> type = RESPONSE_PL;
    return MSE_OK;
  default:
    if (MS_errno == MSE_BADREQUEST) {
      if (__response_init (response, "400 bad request", NULL) != MSE_OK)
        goto ServerError;
      return MSE_OK;
    }
    goto ServerError;
  }

 ServerError:
   if (__response_init (response, "500 server error", NULL) != MSE_OK)
     return MS_errno;
   return MSE_OK;
}

/* write onto fd "bytes" bytes of "buffer" */
static int
Write (int fd, char *buffer, ssize_t bytes)
{
  ssize_t bytes_written;

  while ((bytes_written = write (fd, buffer, bytes)) >= 0)
    if (!(bytes -= bytes_written))
      break;
  if (bytes_written < 0)
    return (MS_errno = MSE_WRITERESPONSE);

  return MSE_OK;
}

/* write the http response upon the accepted connection */
int
write_response (int connfd, HTTPResponse response)
{
  ssize_t bytes_to_write, bytes_read;
  char *transmit, *head, buffer [BUFFERSIZE];
  dhlist cur;
  int i;

  /* write http version and response code */
  transmit = Sprintf ("%s %s\r\n", response->version, response->response_code);
  if (transmit == NULL)
    return (MS_errno = MSE_NOMEM);
  bytes_to_write = strlen (transmit);
  if (Write (connfd, transmit, bytes_to_write) != MSE_OK) {
    free (transmit);
    return MS_errno;
  }
  free (transmit);

  /* write headers */
  for (cur = dhlist_first (response -> headers); 
       cur != dhlist_end (response -> headers); cur = dhlist_next (cur)) {
    head = (char *) dhlist_data (cur);
    if ((transmit = Sprintf ("%s\r\n", head)) == NULL) {
      free (transmit);
      return (MS_errno = MSE_NOMEM);
    }
    bytes_to_write = strlen (transmit);
    if (Write (connfd, transmit, bytes_to_write) != MSE_OK) {
      free (transmit);
      return MS_errno;
    }
    free (transmit);
  }

  if (Write (connfd, "\r\n", strlen ("\r\n")) != MSE_OK)
    return MS_errno;

  switch (response -> type) {
  case RESPONSE_FD: /* if message body is a file */
    memset (buffer, '\0', BUFFERSIZE);
    /* transmit file */
    while ((bytes_read = 
              read (*(int*) (response -> body), buffer, BUFFERSIZE)) > 0)
      if (Write (connfd, buffer, bytes_read) != MSE_OK)
        return MS_errno;
      else memset (buffer, '\0', bytes_read);
    if (bytes_read < 0)
      return (MS_errno = MSE_OS);
    return MSE_OK;

  case RESPONSE_PL: /* if message body is just a playlist */
    for (i = 0; i < response -> length; i ++) {
      bytes_to_write = strlen (((char **) (response -> body)) [i]);
      if (Write 
           (connfd, ((char **) (response->body))[i], bytes_to_write) != MSE_OK)
        return MS_errno;
    }
    return MSE_OK;
  case RESPONSE_NO:
    break;
  }
  return MSE_OK;
}

/* print a response informative message to stdout */
void
print_response (char *target, HTTPResponse response)
{
  fprintf (stdout, "[->] sent a %s response to %s under %s.\n",
                   response -> response_code, target, response -> version);
  return;
}

/* free any resources used by HTTP request and response */
void
transaction_done (HTTPRequest request, HTTPResponse response)
{
  dhlist cur;
  int i;

  if (request != NULL) {
    free (request -> command);
    free (request -> resource);
    free (request -> version);
    for (cur = dhlist_first (request -> headers);
         cur != dhlist_end (request -> headers); cur = dhlist_next (cur))
      free (dhlist_data (cur));
    dhlist_delete (request -> headers);
    free (request);
  }

  if (response != NULL) {
    free (response -> version);
    free (response -> response_code);
    for (cur = dhlist_first (response -> headers);
         cur != dhlist_end (response -> headers); cur = dhlist_next (cur))
      free (dhlist_data (cur));
    dhlist_delete (response -> headers);
    switch (response -> type) {
    case RESPONSE_PL:
      for (i = 0; i < response -> length; i ++)
        free (((char **) response -> body) [i]);
      free (response -> body);
      break;
    case RESPONSE_FD:
      close (* (int *) (response -> body));
      free (response -> body);
      break;
    case RESPONSE_NO:
      break;
    }
    free (response);
  }

  return;  
}

