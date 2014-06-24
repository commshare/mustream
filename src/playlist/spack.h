# ifndef __SONG_PACKET_LIB__
# define __SONG_PACKET_LIB__

typedef struct SongPack *spack;

spack  spack_init         (char *, char *);
char*  spack_server_path  (spack);
char*  spack_client_path  (spack);
char*  spack_content      (spack);
int    spack_filter       (void *, void *);
char*  spack_formal       (spack, char *);
void   spack_free         (spack);	

# endif
