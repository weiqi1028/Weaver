all:
	gcc -o weaver rio.c list.c utils.c threadpool.c httprequest.c httphandler.c weaver.c -lpthread

