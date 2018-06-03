# ################## Makefile

default:
	gcc climain.c client.c -lpthread -o clnt
	gcc sermain.c server.c mylist.c sqlitedb.c -lpthread -lsqlite3 -o srvr
clean:
	rm *.o

