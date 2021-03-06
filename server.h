/////////////////////////////////////// server.h

#ifndef _SERVER_H
#define _SERVER_H
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sqlite3.h>
#include <dirent.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <stdbool.h>

enum{SQL_ERROR = -1,SQL_NONE,SQL_FOUND};
typedef struct sockaddr_in SA4;
typedef struct sockaddr SA;

typedef struct node{
	int tcfd;
	char username[32];	
	int logstatus;
	int chatstatus;
	struct node* pprev;
	struct node* pnext;
}node;

typedef struct list{
	node* pcur;
	node head;
	node tail;
}list;

typedef struct listshow{
	int chaters;
	char userlist;
}listshow;

void phelp(void);
void* pcontrol(void*);
void* pnewthread(void* pcfd);
void* plogin(void* pcfd);
void* plogout(void* pcfd);
void* pregister(void* pcfd);
void* pcheckon(void* pcfd);
void* pcheckfiles(void* pcfd);
void* ptalk_transfer(void* pcfd);
void* pquit(void* pcfd);
void pgroupmsg(int mycfd,char* msg,char* myname);
void pfile_upload(int cfd,char* filepath);
void pfile_download(int cfd,char* filepath);
int pbind(int port);
void setnonblock(int sfd);

int list_init();
int list_count();
char* list_names(int cnt,char* names);
int list_getcfd(const char* username);
int* list_getcfdarr(int** pcfdarr,int* pcnt);
char* list_getname(int cfd);
int list_append(int cfd);
int list_login(int cfd,const char* username);
int list_logout(int cfd);
int list_logstatus(int cfd);
int list_chatin(int cfd);
int list_chatout(int cfd);
int list_chatstatus(int cfd);
int list_exit(int cfd);
int list_delete(int cfd);
int list_destroy();

int db_open();
int db_check(const char* username,const char* password);
int db_insert(const char* username,const char* password);
int db_delete(const char* username);
//static int callback(void* data,int argc,char** argv,char** azcolname);

#endif//_SERVER_H
