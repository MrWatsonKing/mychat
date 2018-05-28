#include "client.h"/////////////////////// climain.c

int main(int argc,char** argv){
/*	
	if(argc != 2){
		printf("Usage: clnt <ip>\n");
		return -1;
	}
	int sfd = pconnect(argv[1]);
*/
	int sfd = pconnect("127.0.0.1");
	if(sfd == -1){
		printf("pconnect fails\n");
		return -1;
	}	
	printf("successfully connected.\n");	

	while(1){
		switch(pcommand()){
			case LOGIN:
				plogin(sfd);
				break;
			case LOGOUT:
				plogout();
				break;
			case REGISTER:
				pregister(sfd);
				break;
			case CHECKON:
				pcheckon(sfd);
				break;
			case TALK:
				ptalk(sfd);
				break;
			case QUIT:
				pquit(sfd);
				break;
			case HELP:
				phelp();
				break;
			case UNKNOWN:
				punknown();
				break;
			default:
				break;
		}
	}
	
	return 0;
}

