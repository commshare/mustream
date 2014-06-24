/* spack.c: Song package structure */
# include <stdio.h>
# include <stdlib.h>
# include <string.h>

# include "../sharedlib/url_codec.h"
# include "../sharedlib/strmod.h"
# include "../mstream/mserrors.h"
# include "spack.h"

struct SongPack {     /* a song - entry of the music library */
  char *client_path;  /* the path that will be sent to the client */
  char *server_path;  /* the real path of the song */
  char *content_type; /* the content type of the song */
};

/* check song-type and assign the appropriate content */
static char *
__get_content (char *path)
{
  int len = strlen (path);
  char *content;

  path += len - 4;
  if (!strcmp (path, ".mp3")) {
    if ((content = strdup ("audio/mpeg")) == NULL) {
      MS_errno = MSE_NOMEM;
      return NULL;
    }
  }
  else if (!strcmp (path, ".ogg")) {
    if ((content = strdup ("audio/ogg")) == NULL) { 
      MS_errno = MSE_NOMEM;
      return NULL;
    }
  }
  else if (!strcmp (path, ".aac")) {
    if ((content = strdup ("audio/aac")) == NULL) { 
      MS_errno = MSE_NOMEM;
      return NULL;
    }
  }
  else if (!strcmp (path, ".wma")) {
    if ((content = strdup ("audio/x-ms-wma")) == NULL) { 
      MS_errno = MSE_NOMEM;
      return NULL;
    }
  }
  else if (!strcmp (path, ".m4a")) {
    if ((content = strdup ("audio/mp4a-latm")) == NULL) { 
      MS_errno = MSE_NOMEM;
      return NULL;
    }
  }
  else if (!strcmp (path, ".m4p")) {
    if ((content = strdup ("audio/mp4a-latm")) == NULL) { 
      MS_errno = MSE_NOMEM;
      return NULL;
    }
  }
  else if (!strcmp (path, ".m3u")) {
    if ((content = strdup ("audio/x-mpegurl")) == NULL) { 
      MS_errno = MSE_NOMEM;
      return NULL;
    }
  }
  else {
    if ((content = strdup ("audio/x-flac")) == NULL) { 
      MS_errno = MSE_NOMEM;
      return NULL;
    }
  }

  return content;
}

/* initialise a song entry */
spack
spack_init (char *path, char *musicdir)
{
  spack song;
  int len;

  if ((song = (spack) malloc (sizeof (struct SongPack))) == NULL) {
    MS_errno = MSE_NOMEM;
    return NULL;
  }

  /* 
   * the client path will be saved in a url encoded format, so that each
   * request is handled in a direct manner (no decoding takes place).
   */
  len = strlen (path + strlen (musicdir));
  song -> client_path = NULL;
  do {
    if (song -> client_path != NULL)
      free (song -> client_path);
    len += 5;
    if ((song -> client_path = calloc (len, sizeof(char))) == NULL) {
      MS_errno = MSE_NOMEM;
      free (song);
      return NULL;
    }
  } while (url_encode(path + strlen(musicdir), song->client_path, len, 1) < 0);

  if ((song -> server_path = strdup (path)) == NULL) {
    MS_errno = MSE_NOMEM;
    free (song -> client_path);
    free (song);
    return NULL;
  }
  if ((song -> content_type = __get_content (path)) == NULL) {
    free (song -> server_path);
    free (song -> client_path);
    free (song);
    return NULL;
  }

  return song;
}

char *
spack_server_path (spack song)
{
  return song -> server_path;
}

char *
spack_client_path (spack song)
{
  return song -> client_path;
}

char *
spack_content (spack song)
{
  return song -> content_type;
}

/* filters song entries against a given path */
int
spack_filter (void *spackop, void *path)
{
  char *Path = (char *) path;
  spack song = (spack) spackop;

  return strcmp (Path, song -> client_path);
}

/* formalise the client path, so that it can be sent */
char *
spack_formal (spack song, char *host)
{
  char *res;

  res = Sprintf ("http://%s%s\n", host, song -> client_path);
  if (res == NULL)
    MS_errno = MSE_NOMEM;

  return res;
}

void
spack_free (spack song)
{
  free (song -> client_path);
  free (song -> server_path);
  free (song -> content_type);
  free (song);
  return;
}

