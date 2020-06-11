#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h> /* sockets */
#include <sys/types.h> /* sockets */
#include <sys/socket.h> /* sockets */
#include <netinet/in.h> /* internet sockets */
#include <arpa/inet.h>
#include <netdb.h> /* ge th os tb ya dd r */
#include <unistd.h> /* fork */
#include <stdlib.h> /* exit */
#include <ctype.h> /* toupper */
#include <signal.h> /* signal */
#define err(mess){fprintf(stderr,"\033[1;31mERROR: \033[0m: %s\n",mess);exit(1);}

int main(int argc, char const *argv[]){

	char queryFile[32],servIP[32];
	int numThreads,servPort;
	FILE* fp;
	char* line = NULL;
	size_t len = 0;
	char buffer[256];
	int sock;

	struct sockaddr_in server;
	struct sockaddr * serverptr = (struct sockaddr *) &server;
	struct hostent * rem;
	struct in_addr server_addr;

	for(int i=0; i<argc;i++){
		
		if(!strcmp(argv[i],"-q"))
			strcpy(queryFile,argv[i+1]);
		if(!strcmp(argv[i],"-sp"))
			servPort = atoi(argv[i+1]);
		if(!strcmp(argv[i],"-sip"))
			strcpy(servIP,argv[i+1]);
		if(!strcmp(argv[i],"-w"))
			numThreads =  atoi(argv[i+1]);
	}

	inet_aton(servIP,&server_addr);

	if((fp = fopen(queryFile,"r")) == NULL)	//open file
		err("Can not open file");

	while(!feof(fp)){
		getline(&line,&len,fp);		
	}

	if((sock = socket(AF_INET, SOCK_STREAM,0)) < 0)			/* Create socket */
		err("Socket");

	if((rem = gethostbyaddr((const char*)&server_addr,sizeof(server_addr),AF_INET)) == NULL)		/* Find server address */
		err("gethostbyaddr");
	
	server.sin_family = AF_INET;		 /* Internet domain */
	memcpy(&server.sin_addr,rem->h_addr,rem->h_length);
	server.sin_port = htons(servPort);	 /* Server port */

	if(connect(sock,serverptr,sizeof(server)) < 0)
		err("Connect");
	
	printf("Connecting to %s IP in port %d \n",servIP, servPort);

	do{
		printf("Write: ");		
		scanf("%s",buffer); /* Read from stdin */
		
		if(write(sock,buffer,sizeof(buffer)) < 0)
			err("write");
		
		if(read(sock,buffer,sizeof(buffer)) < 0)
			err("read");
		
		printf("Received string: %s\n",buffer);

	}while(strcmp(buffer,"END \n")!= 0);		 /* Finish on " end " */
	
	close(sock); 	/* Close socket and exit */
	
	return 0;
}