#include "server.h" //mylist.c
extern list users;
extern int nthreads;

int list_init(){

	users.pcur = NULL;
	users.head.pprev = NULL;
	users.tail.pnext = NULL;
	users.head.pnext = &users.tail;
	users.tail.pprev = &users.head;

	printf("list_init successful.\n");
	return 0;
}

int list_count(const char* what){
	
	int cnt = 0;
	node* pnode = NULL;
	for(pnode = users.head.pnext; pnode != &users.tail; pnode = pnode->pnext)
		if(pnode != &users.tail){
			if(!strcmp(what,"threads"))
				cnt++;
			else if(!strcmp(what,"logins") && pnode->logstatus == 1)
				cnt++;
			else if(!strcmp(what,"chaters") && pnode->logstatus == 1 && pnode->chatstatus == 1)
				cnt++;
		}			
	
	return cnt;
}

int list_show(){
	
	//获取最新时间
    time_t t = 0;
    struct tm *today = NULL;
    t = time(NULL);
    today = localtime(&t);
    
    int chaters = list_count("chaters");
	printf("chaters online: %d\n",chaters);

    if(chaters == 0){
        printf("[%02d:%02d:%02d]\n\n",today->tm_hour,today->tm_min,today->tm_sec);
        return 0;
    }
    
    char* userlist= (char*)malloc(32*chaters);
    if(userlist == NULL){        
        printf("failed to malloc for userlist.\n");
        return -1;
    }
    userlist[0] = '\0';

    int i = 0;
    int lensum = 0;	
	node* pnode = NULL;    
    //如果plist->head.pnext == &plist->tail,即plist当中没有有效成员的话,就不会进行循环
    for(pnode = users.head.pnext; pnode != &users.tail; pnode = pnode->pnext){
		if(i<chaters){
			strcat(userlist,pnode->username);
			strcat(userlist," ");
			lensum += strlen(pnode->username)+1;
			i++;
		}
    }
    userlist[lensum] = '\0';    
    printf("%s\n",userlist);
    printf("[%02d:%02d:%02d]\n\n",today->tm_hour,today->tm_min,today->tm_sec);
    
    free(userlist);
    userlist = NULL;
	
	return chaters;
}

int list_getcfd(const char* username){
	
	node* pnode = NULL;
	for(pnode = users.head.pnext; pnode != &users.tail; pnode = pnode->pnext)
		if(!strcmp(username,pnode->username))
			return pnode->tcfd;
	
	return -1;
}

int* list_getcfdarr(int** pcfdarr,int* pcnt){
	*pcnt = list_count("chaters");
	*pcfdarr = (int*)malloc(sizeof(int) * (*pcnt));
	if(*pcfdarr == NULL){
		printf("failed to malloc memory to init cfdarr[clients].\n");
		return NULL;
	}
	int i = 0;
	node* pnode = NULL;
	for(pnode = users.head.pnext; pnode != &users.tail; pnode = pnode->pnext)
		if(i < *pcnt && pnode->logstatus == 1 && pnode->chatstatus == 1)
			(*pcfdarr)[i++] = pnode->tcfd;

	return *pcfdarr;
}

char* list_getname(int cfd){
	
	node* pnode = NULL;
	for(pnode = users.head.pnext; pnode != &users.tail; pnode = pnode->pnext)
		if(cfd == pnode->tcfd)
			return pnode->username;
	
    printf("failed to get name from list where cfd=%d\n",cfd);
	return NULL;
}

int list_append(int cfd,pthread_t tid){

	node* pnode = (node*)malloc(sizeof(node));
	if(pnode == NULL){
		printf("\nfailed to append cfd=%d into list.\n",cfd);
		return -1;
	}
	memset(pnode,0,sizeof(node));	
	pnode->tcfd = cfd;
	pnode->tid = tid;
	users.tail.pprev->pnext = pnode;
	pnode->pprev = users.tail.pprev;
	pnode->pnext = &users.tail;
	users.tail.pprev = pnode;
	nthreads++;
//	printf("user cfd=%d appended to list successful.\n",cfd);
	return 0;
}

int list_login(int cfd,const char* username){
	node* pnode = NULL;
	for(pnode = users.head.pnext; pnode != &users.tail; pnode = pnode->pnext)
		if(cfd == pnode->tcfd){			
			strcpy(pnode->username,username);
			pnode->logstatus = 1;
			//printf("user cfd=%d login successful.\n",cfd);
			return 0;
		}
	printf("user thread cfd=%d does not exist.\n",cfd);
	return -1;
}

int list_logout(int cfd){
	node* pnode = NULL;
	for(pnode = users.head.pnext; pnode != &users.tail; pnode = pnode->pnext)
		if(pnode->tcfd == cfd){
			memset(pnode->username,0,32);			
			pnode->logstatus = 0;
			//printf("user cfd=%d logouted successful.\n",cfd);
			return 0;
		}
	printf("user thread cfd=%d does not exist.\n",cfd);
	return -1;
}

int list_logstatus(int cfd){
	node* pnode = NULL;
	for(pnode = users.head.pnext; pnode != &users.tail; pnode = pnode->pnext)
		if(pnode->tcfd == cfd){			
			return pnode->logstatus;
		}
	printf("user thread cfd=%d does not exist.\n",cfd);
	return -1;
}

int list_chatin(int cfd){
	node* pnode = NULL;
	for(pnode = users.head.pnext; pnode != &users.tail; pnode = pnode->pnext)
		if(pnode->tcfd == cfd){
			pnode->chatstatus = 1;
			//printf("user cfd=%d enterd chatroom successful.\n",cfd);
			return 0;
		}
	printf("user thread cfd=%d does not exist.\n",cfd);
	return -1;
}

int list_chatout(int cfd){
	node* pnode = NULL;
	for(pnode = users.head.pnext; pnode != &users.tail; pnode = pnode->pnext)
		if(pnode->tcfd == cfd){
			pnode->chatstatus = 0;
			//printf("user cfd=%d enterd chatroom successful.\n",cfd);
			return 0;
		}
	printf("user thread cfd=%d does not exist.\n",cfd);
	return -1;
}

int list_chatstatus(int cfd){
	node* pnode = NULL;
	for(pnode = users.head.pnext; pnode != &users.tail; pnode = pnode->pnext)
		if(pnode->tcfd == cfd){			
			return pnode->chatstatus;
		}
	printf("user thread cfd=%d does not exist.\n",cfd);
	return -1;
}

int list_exit(int cfd){
	node* pnode = NULL;
	for(pnode = users.head.pnext; pnode != &users.tail; pnode = pnode->pnext){
		if(cfd == pnode->tcfd){			
			pnode->pprev->pnext = pnode->pnext;
			pnode->pnext->pprev = pnode->pprev;
			free(pnode);
			pnode = NULL;
			nthreads--;				
//			printf("\nuser cfd=%d deleted from list successful.\n",cfd);
			return 0;
		}
	}
	printf("user cfd=%d does not exist!\n",cfd);
	return -1;
}

int list_delete(int cfd){

	node* pnode = NULL;
	for(pnode = users.head.pnext; pnode != &users.tail; pnode = pnode->pnext){
		if(pnode->tcfd == cfd){
			if(!pthread_cancel(pnode->tid)){
				nthreads--;
				printf("client thread cfd=%d canceled.\ttotal threads: %d\n",cfd,nthreads);
			}				
			pnode->pprev->pnext = pnode->pnext;
			pnode->pnext->pprev = pnode->pprev;
			free(pnode);
			pnode = NULL;
//			printf("\nuser cfd=%d deleted from list successful.\n",cfd);
			return 0;
		}
	}
	printf("user cfd=%d does not exist!\n",cfd);
	return -1;
}

int list_destroy(){

	users.pcur = NULL;
	while(users.head.pnext != &users.tail){
		node* pfirst = &users.head;
		node* pmid = pfirst->pnext;
		node* plast = pmid->pnext;
		
		pfirst->pnext = plast;
		plast->pprev = pfirst;
		free(pmid);
		pmid = NULL;
	}

	return 0;
}

