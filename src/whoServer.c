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
#include <pthread.h>
#include <poll.h>
#include "../include/utils.h"
#define err(mess){fprintf(stderr,"\033[1;31mERROR: \033[0m: %s\n",mess);exit(1);}

typedef struct{
	int fd;
	char info[5];
}fd_info;

typedef struct {
	fd_info* data;
	int start;
	int end;
	int count;
}buffer_t;

buffer_t buffer;

pthread_mutex_t mtx;
pthread_mutex_t print_mtx;
pthread_cond_t cond_nonempty;
pthread_cond_t cond_nonfull;

void initialize(buffer_t * buffer,int num){
	buffer->start = 0;
	buffer->end = -1;
	buffer->count = 0;
	buffer->data = malloc(num*sizeof(fd_info));
}

void deleteBuffer(buffer_t* buffer){
	free(buffer->data);
}

void addFd(buffer_t* buffer,int fd,char* info,int bufferSize){
	pthread_mutex_lock(&mtx);
	while(buffer->count >=bufferSize){
		pthread_cond_wait(&cond_nonfull, &mtx);
	}
	buffer->end = (buffer->end+1)%bufferSize;
	buffer->data[buffer->end].fd = fd;
	strcpy(buffer->data[buffer->end].info, info);
	buffer->count++;
	pthread_mutex_unlock(&mtx);
}

fd_info returnFdInfo(buffer_t* buffer,int bufferSize){
	fd_info data;
	pthread_mutex_lock(&mtx);
	while(buffer->count <= 0){
		pthread_cond_wait(&cond_nonempty, &mtx);
	}
	data = buffer->data[buffer->start];
	buffer->start = (buffer->start+1)%bufferSize;
	buffer->count--;
	pthread_mutex_unlock(&mtx);
	return data;
}

void * thread_function(void * arg){
	fd_info data;
	int bufferSize = *(int*) arg; 
	printf("I'm thread\n");
	data = returnFdInfo(&buffer,bufferSize);
	pthread_cond_signal(&cond_nonfull);

	if(!strcmp(data.info,"q")){
		printf("Receive Query\n");
		char buffer[150];
		while(read(data.fd,buffer,sizeof(buffer))>0){
			pthread_mutex_lock(&print_mtx);
			printf("-- %s\n",buffer);
			pthread_mutex_unlock(&print_mtx);
		}

	}else if(!strcmp(data.info,"s")){
		/*  Receive Statistics */ 
		printf("Receive Statistics\n");
		statistics* stat = calloc(1,sizeof(statistics));
		
		while(read(data.fd,stat,sizeof(statistics))>0){		
			pthread_mutex_lock(&print_mtx);
			printStat(stat);
			pthread_mutex_unlock(&print_mtx);
		}

	}else
		err("problem with fd info");
}

int main(int argc, char const *argv[]){

	int queryPortNum,statisticsPortNum;
	int numThreads,bufferSize;
	int query_fd,statistics_fd, answer_fd;
	struct sockaddr_in query_server, statistics_server,worker;

	struct sockaddr *query_serverptr=(struct sockaddr *) &query_server;
	struct sockaddr *statistics_serverptr=(struct sockaddr *) &statistics_server;
	struct sockaddr *workerptr=(struct sockaddr *) &worker;

	struct hostent *rem;
	socklen_t workerlen;
	char buff[64];
	int err;
	pthread_t * tids;

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

	/*------------------------- Create socket for queries -------------------------*/

	if((query_fd = socket(AF_INET , SOCK_STREAM ,0)) < 0)			/* Create socket */
		err("Socket");

	query_server.sin_family = AF_INET; 		/* Internet domain */
	query_server.sin_addr.s_addr = htonl(INADDR_ANY);
	query_server.sin_port = htons(queryPortNum); 		/* The given port */
	
	if(bind(query_fd,query_serverptr,sizeof(query_server)) < 0)		/* Bind socket to address */
		err("Bind");
		
	if(listen(query_fd,5) < 0)		/* Listen for connections */
		err("Listen");
	

	/*---------------------- Create socket for statistics -------------------------*/

	if((statistics_fd = socket(AF_INET , SOCK_STREAM ,0)) < 0)			/* Create socket */
		err("Socket");

	statistics_server.sin_family = AF_INET; 		/* Internet domain */
	statistics_server.sin_addr.s_addr = htonl(INADDR_ANY);
	statistics_server.sin_port = htons(statisticsPortNum); 		/* The given port */
	
	if(bind(statistics_fd,statistics_serverptr,sizeof(statistics_server)) < 0)		/* Bind socket to address */
		err("Bind");
		
	if(listen(statistics_fd,5) < 0)		/* Listen for connections */
		err("Listen");
	
	/*-----------------------------------------------------------------------------*/

	int rc;
	struct pollfd *socket_fds;
	socket_fds = calloc(2,sizeof(struct pollfd));
	socket_fds[0].fd = query_fd;		// port for queries
	socket_fds[0].events = POLLIN;
	socket_fds[1].fd = statistics_fd;	// port for statistics
	socket_fds[1].events = POLLIN;

	initialize(&buffer,bufferSize);
	pthread_mutex_init(&mtx,0);
	pthread_mutex_init(&print_mtx,0);
	pthread_cond_init(&cond_nonempty,0);
	pthread_cond_init(&cond_nonfull,0);

	/*---------------------------- Create Threads -----------------------------------*/
	

	if((tids = malloc(numThreads*sizeof(pthread_t))) == NULL)
		err("malloc");

	for(int i=0 ; i < numThreads ; i++){
		printf("Create thread\n");
		if(pthread_create(tids+i, NULL ,thread_function, (void*) &bufferSize)<0)
			err("create thread");
	}

	/*-----------------------------------------------------------------------------*/

	printf("start poll\n");

	while(1){
		rc = poll(socket_fds,2,-1);	
		if(rc > 0){
			if((socket_fds[0].revents & POLLIN)){				// if a queryPort has request			
				if((answer_fd = accept(query_fd,NULL,0)) < 0)	/* accept connection */
					err("Accept");
				printf("accept connection with client\n");
				addFd(&buffer,answer_fd,"q",bufferSize);
				pthread_cond_signal(&cond_nonempty);

			}else if((socket_fds[1].revents & POLLIN)){			// if statistics have arrived
				if((answer_fd = accept(statistics_fd,workerptr,&workerlen)) < 0)	/* accept connection */
					err("Accept");
				printf("accept connection with worker\n");
				addFd(&buffer,answer_fd,"s",bufferSize);
				pthread_cond_signal(&cond_nonempty);
			}
		}
	}

	for(int i=0 ; i < numThreads ; i++){		/* Wait for thread termination */
		if((err=pthread_join(*(tids + i),NULL))<0) 
			err("pthread_join");

	}
	close(query_fd);
	close(statistics_fd);
	pthread_cond_destroy(&cond_nonempty);
	pthread_cond_destroy(&cond_nonfull);
	pthread_mutex_destroy(&mtx);

	return 0;
}