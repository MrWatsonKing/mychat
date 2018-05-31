#include "server.h" //list.c

int list_init(list* plist){

	plist->pcur = NULL;
	plist->head.pprev = NULL;
	plist->tail.pnext = NULL;
	plist->head.pnext = &plist->tail;
	plist->tail.pprev = &plist->head;

	printf("list_init successful.\n");
	return 0;
}

int list_count(list* plist){
	
	int cnt = 0;
	node* pnode = NULL;
	for(pnode = plist->head.pnext; pnode != &plist->tail; pnode = pnode->pnext)
		if(pnode != &plist->tail)
			cnt++;
	
	return cnt;
}

int list_show(list* plist,int cfd){
	
	int cnt = list_count(plist);
	dprintf(cfd,"%d\n",cnt);
	if(cnt == 0) return 0;

	char* userlist= (char*)malloc(32*cnt+100);//彻底杜绝内存不足? 32不是已经够了吗?
	if(userlist == NULL){
		dprintf(cfd,"failed to get userlist.\n");
		printf("failed to get userlist.\n");
		return -1;
	}
	
	//使用strcat()之前，一定要bzero.bzero这个函数貌似经常出错
	userlist[0] = '\0';

	node* pnode = NULL;
	int lensum = 0;
	//如果plist->head.pnext == &plist->tail,即plist当中没有有效成员的话,就不会进行循环
	for(pnode = plist->head.pnext; pnode != &plist->tail; pnode = pnode->pnext){
		strcat(userlist,pnode->username);
		strcat(userlist," ");
		lensum += strlen(pnode->username)+1;
	}
	userlist[lensum-1] = '\0';
	dprintf(cfd,"%s\n",userlist);//userlist发出去之后包含\n
	//printf("userlist sent:%s\n",userlist);
	free(userlist);
	userlist = NULL;
	return 0;
}

int list_getcfd(const char* username,list* plist){
	
	node* pnode = NULL;
	for(pnode = plist->head.pnext; pnode != &plist->tail; pnode = pnode->pnext)
		if(!strcmp(username,pnode->username))
			return pnode->tcfd;
	
	return -1;
}

int* list_getcfdarr(int** pcfdarr,int* pcnt,list* plist){
	*pcnt = list_count(plist);
	*pcfdarr = (int*)malloc(sizeof(int) * (*pcnt));
	if(*pcfdarr == NULL){
		printf("failed to malloc mem to init cfdarr[clients].\n");
		return NULL;
	}
	int i = 0;
	node* pnode = NULL;
	for(pnode = plist->head.pnext; pnode != &plist->tail; pnode = pnode->pnext)
		(*pcfdarr)[i++] = pnode->tcfd;

	return *pcfdarr;
}

char* list_getname(int cfd,list* plist){
	
	node* pnode = NULL;
	for(pnode = plist->head.pnext; pnode != &plist->tail; pnode = pnode->pnext)
		if(cfd == pnode->tcfd)
			return pnode->username;
	
//	printf("failed to get name from list where cfd=%d\n",cfd);
	return NULL;
}

int list_append(const char* username,int cfd,list* plist){
	
	node* pnode = (node*)malloc(sizeof(node));
	if(pnode == NULL){
		printf("\nfailed to append %s into list.\n",username);
		return -1;
	}

	strcpy(pnode->username,username);
	pnode->tcfd = cfd;
	plist->tail.pprev->pnext = pnode;
	pnode->pprev = plist->tail.pprev;
	pnode->pnext = &plist->tail;
	plist->tail.pprev = pnode;
//	printf("user cfd=%d appended to list successful.\n",cfd);

	return 0;
}

int list_delete(int cfd,list* plist){

	node* pnode = NULL;
	for(pnode = plist->head.pnext; pnode != &plist->tail; pnode = pnode->pnext){
		if(cfd == pnode->tcfd){
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

int list_destroy(list* plist){

	plist->pcur = NULL;
	while(plist->head.pnext != &plist->tail){
		node* pfirst = &plist->head;
		node* pmid = pfirst->pnext;
		node* plast = pmid->pnext;
		
		pfirst->pnext = plast;
		plast->pprev = pfirst;
		free(pmid);
		pmid = NULL;
	}

	return 0;
}

