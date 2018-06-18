#include "server.h" // mylist.c

extern list users;
extern int nthreads;
pthread_rwlock_t list_rwlock = PTHREAD_RWLOCK_INITIALIZER;

int list_init(){
	//list初始化 仅当服务器创建时调用
	users.pcur = NULL;
	users.head.pprev = NULL;
	users.tail.pnext = NULL;
	users.head.pnext = &users.tail;
	users.tail.pprev = &users.head;

	printf("list_init successful.\n");
	return 0;
}

int list_count(const char* what){	

	if(strcmp(what,"threads") && strcmp(what,"logins") && strcmp(what,"chaters")){
		printf("list_count:\"threads\",\"logins\",\"chaters\"; \"%s\" is not recognized.\n",what);
		return 0;
	}

	int cnt = 0;
	node* pnode = NULL;
	pthread_rwlock_rdlock(&list_rwlock);
	for(pnode = users.head.pnext; pnode != &users.tail; pnode = pnode->pnext)
		if(pnode != &users.tail){			
			if(!strcmp(what,"threads"))
				cnt++;
			else if(!strcmp(what,"logins") && pnode->logstatus == 1)
				cnt++;
			else if(!strcmp(what,"chaters") && pnode->logstatus == 1 && pnode->chatstatus == 1)
				cnt++;
		}

	pthread_rwlock_unlock(&list_rwlock);
	return cnt;
}

char* list_names(int cnt,char* names){
	//names为外部传入的char数组，长度为cnt*32	
	if(cnt == 0){
		printf("chaters online: 0\n\n");
		return NULL;
	}
	if(names == NULL){
		printf("names[] should be malloced 32*cnt bytes before use.\n");
		return NULL;
	}

	int i = 0;
	node* pnode = NULL;  
	names[0] = '\0';
    //如果plist->head.pnext == &plist->tail,即plist当中没有有效成员的话,就不会进行循环
	pthread_rwlock_rdlock(&list_rwlock);
    for(pnode = users.head.pnext; pnode != &users.tail; pnode = pnode->pnext){
		//只对list当中前cnt个操作，在操作期间通过append()方式新增的用户，不计入其中
		if(i<cnt){
			strcat(names,pnode->username);
			strcat(names,"  ");
			i++;
		}
    }

	pthread_rwlock_unlock(&list_rwlock);
	return names;	
}

int list_getcfd(const char* username){
	
	node* pnode = NULL;
	pthread_rwlock_rdlock(&list_rwlock);
	for(pnode = users.head.pnext; pnode != &users.tail; pnode = pnode->pnext)
		if(!strcmp(username,pnode->username)){
			pthread_rwlock_unlock(&list_rwlock);
			return pnode->tcfd;
		}
			
	pthread_rwlock_unlock(&list_rwlock);
	return -1;
}

int* list_getcfdarr(int** pcfdarr,int* pcnt){
	//涉及到内存分配的问题 一定要弄到读写锁里面去 不然内存分配好了结果list_destroy了 那就麻烦了
	*pcnt = list_count("chaters");
	pthread_rwlock_rdlock(&list_rwlock);
	*pcfdarr = (int*)malloc(sizeof(int) * (*pcnt));
	if(*pcfdarr == NULL){
		pthread_rwlock_unlock(&list_rwlock);
		printf("failed to malloc memory to init cfdarr[clients].\n");
		return NULL;
	}
	int i = 0;
	node* pnode = NULL;	
	for(pnode = users.head.pnext; pnode != &users.tail; pnode = pnode->pnext)
		if(i < *pcnt && pnode->logstatus == 1 && pnode->chatstatus == 1)
			(*pcfdarr)[i++] = pnode->tcfd;

	pthread_rwlock_unlock(&list_rwlock);
	return *pcfdarr;
}

char* list_getname(int cfd){
	
	node* pnode = NULL;
	pthread_rwlock_rdlock(&list_rwlock);
	for(pnode = users.head.pnext; pnode != &users.tail; pnode = pnode->pnext)
		if(cfd == pnode->tcfd){
			pthread_rwlock_unlock(&list_rwlock);
			return pnode->username;
		}			
	pthread_rwlock_unlock(&list_rwlock);
    printf("failed to get name from list where cfd=%d\n",cfd);
	return NULL;
}

int list_append(int cfd,pthread_t tid){
	//确保服务器退出注销list的时候，不存在已经分配但没有加入链表的内存
	pthread_rwlock_wrlock(&list_rwlock);
	//接入客户套接字，即开始将客户添加到链表，需要分配内存
	node* pnode = (node*)malloc(sizeof(node));
	if(pnode == NULL){
		pthread_rwlock_unlock(&list_rwlock);
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

	pthread_rwlock_unlock(&list_rwlock);
//	printf("user cfd=%d appended to list successful.\n",cfd);
	return 0;
}

int list_login(int cfd,const char* username){
	//只改变在线用户的登录状态 不增加用户 不分配内存
	node* pnode = NULL;
	pthread_rwlock_wrlock(&list_rwlock);
	for(pnode = users.head.pnext; pnode != &users.tail; pnode = pnode->pnext)
		if(cfd == pnode->tcfd){			
			strcpy(pnode->username,username);
			pnode->logstatus = 1;
			pthread_rwlock_unlock(&list_rwlock);
			//printf("user cfd=%d login successful.\n",cfd);
			return 0;
		}
	
	pthread_rwlock_unlock(&list_rwlock);
	printf("user thread cfd=%d does not exist.\n",cfd);
	return -1;
}

int list_logout(int cfd){
	//只改变在线用户的登录状态 不删除用户 不释放内存
	node* pnode = NULL;
	pthread_rwlock_wrlock(&list_rwlock);
	for(pnode = users.head.pnext; pnode != &users.tail; pnode = pnode->pnext)
		if(pnode->tcfd == cfd){
			memset(pnode->username,0,32);			
			pnode->logstatus = 0;
			pthread_rwlock_unlock(&list_rwlock);
			//printf("user cfd=%d logouted successful.\n",cfd);
			return 0;
		}
	pthread_rwlock_unlock(&list_rwlock);
	printf("user thread cfd=%d does not exist.\n",cfd);
	return -1;
}

int list_logstatus(int cfd){
	//返回登录状态
	node* pnode = NULL;
	pthread_rwlock_rdlock(&list_rwlock);
	for(pnode = users.head.pnext; pnode != &users.tail; pnode = pnode->pnext)
		if(pnode->tcfd == cfd){
			pthread_rwlock_unlock(&list_rwlock);			
			return pnode->logstatus;
		}
	
	pthread_rwlock_unlock(&list_rwlock);
	printf("user thread cfd=%d does not exist.\n",cfd);
	return -1;
}

int list_chatin(int cfd){
	//只改变用户的聊天状态 不添加用户 不分配内存
	node* pnode = NULL;
	pthread_rwlock_wrlock(&list_rwlock);
	for(pnode = users.head.pnext; pnode != &users.tail; pnode = pnode->pnext)
		if(pnode->tcfd == cfd){
			pnode->chatstatus = 1;
			pthread_rwlock_unlock(&list_rwlock);
			//printf("user cfd=%d enterd chatroom successful.\n",cfd);
			return 0;
		}
	pthread_rwlock_unlock(&list_rwlock);
	printf("user thread cfd=%d does not exist.\n",cfd);
	return -1;
}

int list_chatout(int cfd){
	//只改变用户的聊天状态 不添加用户 不分配内存
	node* pnode = NULL;
	pthread_rwlock_wrlock(&list_rwlock);
	for(pnode = users.head.pnext; pnode != &users.tail; pnode = pnode->pnext)
		if(pnode->tcfd == cfd){
			pnode->chatstatus = 0;
			pthread_rwlock_unlock(&list_rwlock);
			//printf("user cfd=%d enterd chatroom successful.\n",cfd);
			return 0;
		}
	pthread_rwlock_unlock(&list_rwlock);
	printf("user thread cfd=%d does not exist.\n",cfd);
	return -1;
}

int list_chatstatus(int cfd){
	//返回聊天状态
	node* pnode = NULL;
	pthread_rwlock_rdlock(&list_rwlock);
	for(pnode = users.head.pnext; pnode != &users.tail; pnode = pnode->pnext)
		if(pnode->tcfd == cfd){
			pthread_rwlock_unlock(&list_rwlock);			
			return pnode->chatstatus;
		}
	pthread_rwlock_unlock(&list_rwlock);
	printf("user thread cfd=%d does not exist.\n",cfd);
	return -1;
}

int list_exit(int cfd){
	//从list当中删除目标客户 但不进行线程操作
	//server.c当中调用此函数之后，会自动跳出循环进入线程终结流程
	node* pnode = NULL;
	pthread_rwlock_wrlock(&list_rwlock);
	for(pnode = users.head.pnext; pnode != &users.tail; pnode = pnode->pnext){
		if(pnode->tcfd == cfd){			
			pnode->pprev->pnext = pnode->pnext;
			pnode->pnext->pprev = pnode->pprev;
			free(pnode);
			pnode = NULL;
			nthreads--;	
			pthread_rwlock_unlock(&list_rwlock);			
//			printf("\nuser cfd=%d deleted from list successful.\n",cfd);
			return 0;
		}
	}
	pthread_rwlock_unlock(&list_rwlock);
	printf("user cfd=%d does not exist!\n",cfd);
	return -1;
}

int list_delete(int cfd){
	//在一个客户服务线程中 从list删除目标客户，并强制结束目标客户对应的服务线程
	//此操作主要用于处理断线残留的、已经停止工作的客户服务线程
	node* pnode = NULL;
	pthread_rwlock_wrlock(&list_rwlock);
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
			pthread_rwlock_unlock(&list_rwlock);
//			printf("\nuser cfd=%d deleted from list successful.\n",cfd);
			return 0;
		}
	}
	pthread_rwlock_unlock(&list_rwlock);
	printf("user cfd=%d does not exist!\n",cfd);
	return -1;
}

int list_destroy(){
	//清空整个list当中的所有客户
	//仅当服务器退出时调用
	pthread_rwlock_wrlock(&list_rwlock);
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
	pthread_rwlock_unlock(&list_rwlock);
	//销毁读写锁
	pthread_rwlock_destroy(&list_rwlock);

	return 0;
}

