/////////////////////////// client.h

#ifndef _CLIENT_H
#define _CLIENT_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sqlite3.h>
#include <time.h>
#include <fcntl.h>
typedef struct sockaddr_in SA4;
typedef struct sockaddr SA;
void phelp(void);
int psendcmd(int sfd);
int plogin(int sfd);
int plogout(int sfd);
int pregister(int sfd);
int pcheckon(int sfd);
int ptalk(int sfd);
int ptalk(int sfd);
void* thread_send(void* psfd);
void* thread_recv(void* psfd);
char* ppath_parse(char* filepath,char* path);
int pfile_recv(int sfd,char* filepath,char* fromname,char* toname);
int pfile_send(int sfd,char* filepath,char* toname);
int pfile_upload(int sfd,char* filepath);
int pfile_download(int sfd,char* filepath);
int pconnect(char* ip);

#endif//_CLIENT_H
