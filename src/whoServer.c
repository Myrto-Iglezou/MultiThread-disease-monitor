#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h> /* sockets */
#include <sys/types.h> /* sockets */
#include <sys/socket.h> /* sockets */
#include <netinet/in.h> /* internet sockets */
#include <netdb.h> /* ge th os tb ya dd r */
#include <unistd.h> /* fork */
#include <stdlib.h> /* exit */
#include <ctype.h> /* toupper */
#include <signal.h> /* signal */
#define err(mess){fprintf(stderr,"\033[1;31mERROR: \033[0m: %s\n",mess);exit(1);}

int main(int argc, char const *argv[]){

	int queryPortNum,statisticsPortNum;
	int numThreads,bufferSize;
	int server_fd, client_fd;
	struct sockaddr_in server,client;

	struct sockaddr *serverptr=(struct sockaddr *) &server;
	struct sockaddr *clientptr=(struct sockaddr *) &client;

	struct hostent *rem;
	socklen_t clientlen;
	char buffer[64];


	for(int i=0; i<argc;i++){
		
		if(!strcmp(argv[i],"-q"))
			queryPortNum = atoi(argv[i+1]);
		if(!strcmp(argv[i],"-b"))
			bufferSize = atoi(argv[i+1]);
		if(!strcmp(argv[i],"-s"))
			statisticsPortNum = atoi(argv[i+1]);
		if(!strcmp(argv[i],"-w"))
			numThreads =  atoi(argv[i+1]);
	}

	if((server_fd = socket( AF_INET , SOCK_STREAM ,0)) < 0)			/* Create socket */
		err("Socket");

	server.sin_family = AF_INET; 		/* Internet domain */
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	server.sin_port = htons(queryPortNum); 		/* The given port */
	
	if(bind(server_fd,serverptr,sizeof(server)) < 0)		/* Bind socket to address */
		err("Bind");
		
	if(listen(server_fd,5) < 0)		/* Listen for connections */
		err("Listen");
	
	printf("Listening for connections to port %d \n",queryPortNum) ;

	if((client_fd = accept(server_fd,NULL,0)) < 0)	/* accept connection */
		err("Accept");

	printf("Accepted connection \n");

	do{
		if(read(client_fd,buffer,sizeof(buffer)) < 0)
			err("Read");
		
		printf("%s\n",buffer );
		
		printf("Write: ");
		scanf("%s",buffer);

		if(write(client_fd,buffer,sizeof(buffer)) < 0)
			err("Write");

	}while(strcmp(buffer,"END")!=0);

	printf("Closing connection .\n");
	close(client_fd); 	/* Close client socket */
	close(server_fd);
	

	return 0;
}