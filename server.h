/////////////////////// server.h

#ifndef _SERVER_H
#define _SERVER_H
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sqlite3.h>
enum{LOGIN = 1,REGISTER,CHECKON,TALK,SENDFILE,QUIT};
enum{SQL_ERROR = -1,SQL_NONE,SQL_FOUND};

typedef struct sockaddr_in SA4;
typedef struct sockaddr SA;

typedef struct node{
	char username[32];
	int tcfd;
	struct node* pprev;
	struct node* pnext;
}node;

typedef struct list{
	node* pcur;
	node head;
	node tail;
}list;

void* pexit(void*);
void* pnewthread(void* pcfd);
int pcommand(int cfd);
int plogin(int cfd,char** pmyname);
int pregister(int cfd);
int pcheckon(int cfd);
int ptalk_transfer(int cfd,char* myname);
void pgroupmsg(int mycfd,char* myname,char* msg);
int pquit(int cfd);
int plisten(int port,int backlog);

int list_init(list* plist);
int list_count(list* plist);
int list_show(list* plist,int cfd);
int list_getcfd(const char* username,list* plist);
int* list_getcfdarr(int** pcfdarr,int* pcnt,list* plist);
char* list_getname(int cfd,list* plist);
int list_append(const char* username,int cfd,list* plist);
int list_delete(int cfd,list* plist);
int list_destroy(list* plist);

int db_open(const char* dbname,sqlite3* pdb);
int db_check(const char* username,const char* password,const char* dbname,sqlite3* pdb);
int db_insert(const char* username,const char* password,const char* dbname,sqlite3* pdb);
int db_delete(const char* username,const char* dbname,sqlite3* pdb);
static int callback(void* data,int argc,char** argv,char** azcolname);

#endif//_SERVER_H
