# ################## Makefile

default:
	gcc -O3 -w climain.c client.c -lpthread -o clnt
	gcc -O3 -w sermain.c server.c mylist.c sqlitedb.c -lpthread -lsqlite3 -o srvr
clean:
	rm *.o

