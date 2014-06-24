/* playlist.c: build & search library */
# include <stdlib.h>
# include <string.h>
# include <dirent.h>

# include "../sharedlib/dhlist.h"
# include "../sharedlib/strmod.h"
# include "../mstream/mserrors.h"
# include "spack.h"

static int
__eliminate_dots (const struct dirent *entry)
{
  return strcmp (entry -> d_name, ".") && strcmp (entry -> d_name, "..");
}

/* check if a file is a song or not */
static int
issong (char *filename)
{
  int len = strlen (filename);

  if (len < 5)
    return 0;

  filename += len - 4;

  return !strcmp (filename, ".mp3")
         || !strcmp (filename, ".ogg")
         || !strcmp (filename, ".aac")
         || !strcmp (filename, ".wma")
         || !strcmp (filename, ".m4a")
         || !strcmp (filename, ".m4p")
         || !strcmp (filename, ".m3u")
         || (len > 6 && !strcmp (filename-1, ".flac"));
}

/*
 * scan the given directory: save each directory entry in the directories
 * list and each song in the files list.
 */
static int
__get_dir_entries (char *directory, dhlist directories, dhlist files)
{
  struct dirent **namelist;
  char *songpath;
  int i, length;

  length = scandir (directory, &namelist, __eliminate_dots, alphasort);
  if (length < 0)
    return (MS_errno = MSE_OS);

  for (i = 0; i < length; i ++)
    if (DT_DIR == namelist [i] -> d_type) { /* if entry is a directory */
      if (!dhlist_append (directories, namelist [i])) {
        MS_errno = MSE_NOMEM;
        goto ErrorEpilogue;
      }
    }
    else if (issong (namelist [i] -> d_name)) {
      songpath = Sprintf ("%s/%s", directory, namelist [i] -> d_name);
      if (songpath == NULL
          || !dhlist_append (files, songpath)) {
        MS_errno = MSE_NOMEM;
        goto ErrorEpilogue;
      }
    }

  return MSE_OK;

 ErrorEpilogue: /* free every resource used in case of error */
  for (i = 0; i < length; i ++)
    free (namelist [i]);
  free (namelist);
  while (dhlist_length (directories))
    dhlist_remove (directories, dhlist_first (directories));
  while (dhlist_length (files))
    dhlist_remove (files, dhlist_first (files));
  return MS_errno;
}

/* get every song contained in a given directory and its subdirectories */
static int
__get_all_songs (char *directory, dhlist *songs)
{
  dhlist dirs, cur, subsongs;
  struct dirent *dir;
  char *name;

  if (!dhlist_init (songs) || !dhlist_init (&dirs))
    return (MS_errno = MSE_NOMEM);

  if (__get_dir_entries (directory, dirs, *songs) != MSE_OK) {
    dhlist_delete (dirs);
    dhlist_delete (*songs);
    return MS_errno;
  }

  if (!dhlist_length (dirs)) {
    dhlist_delete (dirs);
    return MSE_OK;
  }

  /* for each subdirectory */
  for (cur = dhlist_first (dirs); cur != dhlist_end (dirs);
       cur = dhlist_next (cur)) {
    dir = (struct dirent *) dhlist_data (cur);
    if ((name = Sprintf ("%s/%s", directory, dir -> d_name)) == NULL) {
      dhlist_delete (dirs);
      dhlist_delete (*songs);
      return (MS_errno = MSE_NOMEM);
    }
    /* find recursively its songs */
    if (__get_all_songs (name, &subsongs) != MSE_OK) {
      free (name);
      dhlist_delete (dirs);
      dhlist_delete (*songs);
      return MS_errno;
    }
    free (dir);
    free (name);
    dhlist_merge (*songs, subsongs);
  }

  dhlist_delete (dirs);
  return MSE_OK;
}

/*
 * given a directory build the music library by tracking each
 * available song in a list.
 */
int
build_library (char *directory, dhlist songs)
{
  dhlist dirent_songs, cur;
  char *songpath;
  spack songinfo;

  if (__get_all_songs (directory, &dirent_songs) != MSE_OK)
    return MS_errno;

  for (cur = dhlist_first (dirent_songs); cur != dhlist_end (dirent_songs);
       cur = dhlist_next (cur)) {
    songpath = (char *) dhlist_data (cur);
    if ((songinfo = (spack) spack_init (songpath, directory)) == NULL) {
      dhlist_delete (dirent_songs);
      return MS_errno;
    }
    if (!dhlist_append (songs, songinfo)) {
      dhlist_delete (dirent_songs);
      return MS_errno;
    }
    free (songpath);
  }

  dhlist_delete (dirent_songs);
  return MSE_OK;
}

char *__match_key;

int
match_search (void *tomatch)
{
  return strstr (spack_client_path ((spack) tomatch), __match_key) != NULL;
}

/*
 * given the list of available songs, find every song
 * whose path contains key.
 */
int
search_library (dhlist songs, dhlist *search, char *key)
{
  if (key == NULL) { /* if searchstring is empty */
    /* return the whole library */
    if (!dhlist_copy (search, songs))
      return (MS_errno = MSE_NOMEM);
    return MSE_OK;
  }

  __match_key = key;
  if ((*search = dhlist_subset (songs, match_search)) == NULL) {
    __match_key = NULL;
    return (MS_errno = MSE_NOMEM);
  }
  __match_key = NULL;

  return MSE_OK;
}

