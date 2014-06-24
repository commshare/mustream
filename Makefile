#
# muziqstreamer Makefile
# by Yannis Mantzouratos
#

MSTREAMSRC	=	src/mstream/main.c src/mstream/mserrors.c
NETWORKSRC	=	src/network/http.c src/network/serve.c
PLAYLSTSRC	=	src/playlist/playlist.c src/playlist/spack.c
SHAREDLSRC	=	src/sharedlib/dhlist.c src/sharedlib/strmod.c \
			src/sharedlib/url_codec.c

MSTREAMOBJ	=	main.o mserrors.o
NETWORKOBJ	=	http.o serve.o
PLAYLSTOBJ	=	playlist.o spack.o
SHAREDLOBJ	=	dhlist.o strmod.o url_codec.o

MZQSTRMEXEC	=	muziqstreamer

CC = gcc
FLAGS = -c -ggdb

all:		$(MSTREAMOBJ) $(NETWORKOBJ) $(PLAYLSTOBJ) $(SHAREDLOBJ)
		$(CC) $(MSTREAMOBJ) $(NETWORKOBJ) $(PLAYLSTOBJ) $(SHAREDLOBJ) \
		-o $(MZQSTRMEXEC) -lpthread

main.o:		src/mstream/main.c
		$(CC) $(FLAGS) src/mstream/main.c
mserrors.o:	src/mstream/mserrors.c
		$(CC) $(FLAGS) src/mstream/mserrors.c
http.o:		src/network/http.c
		$(CC) $(FLAGS) src/network/http.c
serve.o:	src/network/serve.c
		$(CC) $(FLAGS) src/network/serve.c
playlist.o:	src/playlist/playlist.c
		$(CC) $(FLAGS) src/playlist/playlist.c
spack.o:	src/playlist/spack.c
		$(CC) $(FLAGS) src/playlist/spack.c
dhlist.o:	src/sharedlib/dhlist.c
		$(CC) $(FLAGS) src/sharedlib/dhlist.c
strmod.o:	src/sharedlib/strmod.c
		$(CC) $(FLAGS) src/sharedlib/strmod.c
url_codec.o:	src/sharedlib/url_codec.c
		$(CC) $(FLAGS) src/sharedlib/url_codec.c

clean:
	rm -rf $(MZQSTRMEXEC) $(MSTREAMOBJ) $(NETWORKOBJ) \
	       $(PLAYLSTOBJ) $(SHAREDLOBJ)

clobj:
	rm -rf $(MSTREAMOBJ) $(NETWORKOBJ) $(PLAYLSTOBJ) $(SHAREDLOBJ)

lines:
	wc -l $(MSTREAMSRC) $(NETWORKSRC) $(PLAYLSTSRC) $(SHAREDLSRC)
