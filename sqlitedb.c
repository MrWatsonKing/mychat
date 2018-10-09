#include "server.h" // sqlitedb.c
extern const char* dbname;
extern sqlite3* pdb;
pthread_rwlock_t db_rwlock = PTHREAD_RWLOCK_INITIALIZER;

int db_open(){
	//只在服务器创建的时候调用
	char* sql = NULL;
	char* zerrmsg = NULL;

	if(sqlite3_open(dbname,&pdb) != 0){
		printf("database can not be opened.\n");
		return -1;
	}
	//sql主键只能有一个
	sql	= "create table chaters(\n"
		"username varchar(36) primary key not null,\n"
		"password varchar(36) not null);";
	int rc = sqlite3_exec(pdb,sql,NULL,NULL,&zerrmsg);
	if(rc != SQLITE_OK){
		sqlite3_close(pdb);
		// printf("sql:%s\n",zerrmsg);
		sqlite3_free(zerrmsg);
		return 0;//数据表已经存在而导致键表不成功的情况，应当不妨碍程序的继续运行
	}
	
	sqlite3_close(pdb);
//	printf("table \"chaters\" has been created successfully.\n");
	return 0;
}	

int db_check(const char* username,const char* password){
	//只查询 不删改
	char* zerrmsg = NULL;
	char sql[100] = {0};
	char** pResult = NULL;
	int nRow = 0,nCol = 0;
	
	pthread_rwlock_rdlock(&db_rwlock);
	if(sqlite3_open(dbname,&pdb) != 0){
		pthread_rwlock_unlock(&db_rwlock);
		printf("database can not be opened.\n");
		return -1;
	}

	sprintf(sql,"select * from chaters where username = '%s' and password = '%s';",
			username,password);
	int rc = sqlite3_get_table(pdb,sql,&pResult,&nRow,&nCol,&zerrmsg);
	if(rc != SQLITE_OK){
		sqlite3_close(pdb);
		printf("sql error: %s\n",zerrmsg);
		sqlite3_free(zerrmsg);
		pthread_rwlock_unlock(&db_rwlock);
		return SQL_ERROR;
	}

	sqlite3_free_table(pResult);
	sqlite3_close(pdb);
	pthread_rwlock_unlock(&db_rwlock);

	if(nRow == 0)
		return SQL_NONE;
	
	return SQL_FOUND;
}

int db_insert(const char* username,const char* password){
	//向数据库插入新植
	char* zerrmsg = NULL;
	char sql[100] = {0};

	pthread_rwlock_wrlock(&db_rwlock);
	if(sqlite3_open(dbname,&pdb) != 0){
		pthread_rwlock_unlock(&db_rwlock);
		printf("database can not be opened.\n");
		return -1;
	}

	sprintf(sql,"insert into chaters (username,password) values ('%s','%s');",
			username,password);
	
	int rc = sqlite3_exec(pdb,sql,NULL,NULL,&zerrmsg);
	if(rc != SQLITE_OK){
		sqlite3_close(pdb);
		pthread_rwlock_unlock(&db_rwlock);
		printf("sql error: %s\n",zerrmsg);
		sqlite3_free(zerrmsg);		
		return -1;
	}
	sqlite3_close(pdb);
	pthread_rwlock_unlock(&db_rwlock);
//	printf("%s inserted into table successfully.\n",username);
	return 0;
}

int db_delete(const char* username){
	char* zerrmsg = NULL;
	char sql[100] = {0};
	
	pthread_rwlock_wrlock(&db_rwlock);
	if(sqlite3_open(dbname,&pdb) != 0){
		pthread_rwlock_unlock(&db_rwlock);
		printf("database can not be opened.\n");
		return -1;
	}

	sprintf(sql,"delete * from chaters where username = '%s';",username);
	
	int rc = sqlite3_exec(pdb,sql,NULL,NULL,&zerrmsg);
	if(rc != SQLITE_OK){
		sqlite3_close(pdb);
		pthread_rwlock_unlock(&db_rwlock);
		printf("sql error: %s\n",zerrmsg);
		sqlite3_free(zerrmsg);
		return -1;
	}
	sqlite3_close(pdb);
	pthread_rwlock_unlock(&db_rwlock);
//	printf("%s deleted from table successfully.\n",username);
	return 0;
}

//static int callback(void* data,int argc,char** argv,char** azcolname){
//	for(int i=0; i<argc; i++)
//		printf("%s=%s\n",azcolname[i],argv[i]?argv[i]:NULL);
//	printf("\n");

//	return 0;
//}
