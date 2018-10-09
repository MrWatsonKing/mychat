# ################## Makefile

default:
	gcc -w -O3 climain.c client.c -lpthread -o clnt
	gcc -w -O3 sermain.c server.c mylist.c sqlitedb.c threadpool.c -lpthread -lsqlite3 -o srvr
clean:
	rm *.o

