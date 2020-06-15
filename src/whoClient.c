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
#include <signal.h> /* signal */
#include <pthread.h>
#include "../include/utils.h"
#define err(mess){fprintf(stderr,"\033[1;31mERROR: \033[0m: %s\n",mess);exit(1);}

int sock;

int activeThreads = 0;   
int waitingThreads = 0;  /* number of threads waiting on the condition */
int readyFlag = 0;       /* flag to tell the threads to proceed when signaled */
pthread_cond_t  cond;    /* condition to wait on / signal */
pthread_mutex_t mtx;     /* mutex for the above */
pthread_cond_t  condWaiting; /* variable to signal when each thread starts waiting */
pthread_mutex_t print_mtx;

void * thread_function(void * arg){
	char * line = (char*) arg;

	pthread_mutex_lock(&mtx);

    if (readyFlag == 0) {
      waitingThreads++;
      do{
        pthread_cond_signal(&condWaiting); 
        pthread_cond_wait(&cond,&mtx);
      }while (readyFlag == 0);
      waitingThreads--;
    }
	pthread_mutex_unlock(&mtx);

	write(sock,line,strlen(line)+1);

	pthread_mutex_lock(&print_mtx);

	printf("%s\n",line );

	pthread_mutex_unlock(&print_mtx);
	/* When threads terminate, they decrement the active thread count */
	pthread_mutex_lock(&mtx);
	activeThreads--;
    pthread_cond_signal(&condWaiting); /* signal the main thread when a thread exits to make it recheck the waiting thread count if waiting for all threads to wait */
  	pthread_mutex_unlock(&mtx);
}

int main(int argc, char const *argv[]){

	char queryFile[32],servIP[32];
	int numThreads,servPort;
	FILE* fp;
	char* line = NULL;
	size_t len = 0;
	char buffer[256];
	
	struct sockaddr_in server;
	struct sockaddr * serverptr = (struct sockaddr *) &server;
	struct hostent * rem;
	struct in_addr server_addr;

	pthread_t * tids;

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

	if((fp = fopen(queryFile,"r")) == NULL)	//open file
		err("Can not open file");

	int numOfLines=0;
	char temp[256];
	while(fgets(temp,256,fp)!=NULL){
		numOfLines++;
	}

	fseek(fp,0,SEEK_SET);

	/*---------------------------- Create Threads -----------------------------------*/

	pthread_mutex_init(&mtx,NULL);
	pthread_cond_init(&cond,NULL);
	pthread_cond_init(&condWaiting,NULL); 
	pthread_mutex_init(&print_mtx,0);

	activeThreads = waitingThreads = readyFlag = 0;

	if(numThreads>numOfLines)
		numThreads = numOfLines;

	if((tids = malloc(numThreads*sizeof(pthread_t))) == NULL)
		err("malloc");

	char** lines = malloc(numThreads*sizeof(char*));

	for(int i=0 ; i < numThreads ; i++){
		getline(&line,&len,fp);	
		lines[i] = malloc((strlen(line)+1)*sizeof(char));
		strcpy(lines[i],line);

		if(pthread_create(tids+i, NULL ,thread_function, (void*) lines[i])<0)
			err("create thread");
		activeThreads++;
	}

	pthread_mutex_lock(&mtx);
    
    while (waitingThreads < activeThreads){ /* wait on 'condWaiting' until all active threads are waiting */
      pthread_cond_wait(&condWaiting,&mtx);
    }
    if(waitingThreads != 0){
      readyFlag = 1;
      pthread_cond_broadcast(&cond);
    }

  	pthread_mutex_unlock(&mtx);

  	pthread_mutex_lock(&mtx);
    
    if(waitingThreads == 0)
      readyFlag = 0;
  	
  	pthread_mutex_unlock(&mtx);
	
	for(int i=0 ; i < numThreads ; i++){		/* Wait for thread termination */
		if(pthread_join(*(tids + i),NULL)<0) 
			err("pthread_join");
	}

	// close(sock); 	/* Close socket and exit */
	pthread_cond_destroy(&condWaiting);
	pthread_cond_destroy(&cond);
	pthread_mutex_destroy(&mtx);
	for (int i = 0; i < numThreads; i++){
		free(lines[i]);
	}
	free(lines);
	return 0;
}
