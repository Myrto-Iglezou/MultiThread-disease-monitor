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
#include "./include/utils.h"
#define err(mess){fprintf(stderr,"\033[1;31mERROR: \033[0m: %s\n",mess);exit(1);}

int activeThreads = 0;   
int waitingThreads = 0;  /* number of threads waiting on the condition */
int readyFlag = 0;       /* flag to tell the threads to proceed when signaled */
pthread_cond_t  cond;    /* condition to wait on / signal */
pthread_mutex_t mtx;     /* mutex for the above */
pthread_cond_t  condWaiting; /* variable to signal when each thread starts waiting */

char servIP[32];
int servPort;

void * thread_function(void * arg){
	char * line = (char*) arg;
	char buff[512];
	char s[2] = "$";
	char tempbuffer[150];
	tempbuffer[0] = '\0';
	char temp[100];
	char* token;
	char* tok;
	strcpy(tempbuffer,line);
	strcpy(temp,line);
	int sock;

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

	struct sockaddr_in server;
	struct sockaddr * serverptr = (struct sockaddr *) &server;
	struct hostent * rem;
	struct in_addr server_addr;

	inet_aton(servIP,&server_addr);

	if((sock = socket(AF_INET, SOCK_STREAM,0)) < 0)			/* Create socket */
		err("Socket");

	if((rem = gethostbyaddr((const char*)&server_addr,sizeof(server_addr),AF_INET)) == NULL)		/* Find server address */
		err("gethostbyaddr");
	
	server.sin_family = AF_INET;		 /* Internet domain */
	memcpy(&server.sin_addr,rem->h_addr,rem->h_length);
	server.sin_port = htons(servPort);	 /* Server port */

	if(connect(sock,serverptr,sizeof(server)) < 0)		// connect with server
		err("Connect");
	
	if(write(sock,line,strlen(line)+1)<0)				// write the query
		err("write");

	if(read(sock,buff,sizeof(buff))<0)					// read the answer
		err("read");

	flockfile(stdout);			// lock the stdout, so one thread can write

	token = strtok(temp," ");			// find the command

	if(!strcmp(token,"/topk-AgeRanges")){
		fprintf(stdout,"\033[1;36mREQUEST:  \033[0m%s\n",tempbuffer );
		fflush(stdout);
		char k[10];
		token = strtok(NULL," ");
		strcpy(k,token);
		if(atoi(k)>4)			// if the number requested is greater than the number of the ranges
			strcpy(k,"4");
		tok = strtok(buff,s);
		if(!strcmp(tok,"-1")){
			fprintf(stdout,"\033[1;34mRESULT: \033[0mAn error has occured\n\n");
			fflush(stdout);	
		}else if(!strcmp(tok,"1")){
			fprintf(stdout,"\033[1;34mRESULT:  \033[0m\n");
			fflush(stdout);

			for(int i=0 ; i<2*atoi(k) ; i++){
				tok = strtok(NULL,s);
				if(tok!=NULL && !strcmp(tok,"0"))
					fprintf(stdout,"0-20: ");
				else if(tok!=NULL && !strcmp(tok,"1"))
					fprintf(stdout,"21-40: ");
				else if(tok!=NULL && !strcmp(tok,"2"))
					fprintf(stdout,"41-60: ");
				else if(tok!=NULL && !strcmp(tok,"3"))
					fprintf(stdout,"60+: ");
				else if(tok!=NULL)
					fprintf(stdout, "%s\n",tok);
			}
		}
	}else if(!strcmp(token,"/searchPatientRecord")){
		fprintf(stdout,"\033[1;36mREQUEST:  \033[0m%s\n",tempbuffer );
		fflush(stdout);
		tok = strtok(buff,s);	
		if(!strcmp(tok,"0")){		// if server sends 0, record doesn't exist
			fprintf(stdout,"\033[1;34mRESULT: \033[0m No record with this id\n\n");
			fflush(stdout);	
		}else{
			fprintf(stdout,"\033[1;34mRESULT:  \033[0m");	
			fflush(stdout);	
			tok = strtok(NULL,s);
			while(tok!=NULL){
				fprintf(stdout,"%s ",tok);
				fflush(stdout);	
				tok = strtok(NULL,s);
			}
			fprintf(stdout,"\n\n");
			fflush(stdout);
		}
	}else if(!strcmp(token,"/numPatientAdmissions") || !strcmp(token,"/numPatientDischarges")){
		fprintf(stdout,"\033[1;36mREQUEST:  \033[0m%s\n",tempbuffer );
		fflush(stdout);
		tok = strtok(buff,s);	
		if(strcmp(tok,"-1") && strcmp(tok,"@")){		// if the answer of server is not 1, neither -1 no country is given
			fprintf(stdout,"\033[1;34mRESULT:  \033[0m%s\n\n",tok);	// we expect result for every country
			fflush(stdout);	
			
		}else if(!strcmp(tok,"@")){				// if 1 is send we expect result for a specific country
			tok = strtok(NULL,s);
			fprintf(stdout,"\033[1;34mRESULT:  \033[0m%s\n\n",tok);
			fflush(stdout);
		}else if(!strcmp(tok,"-1")){			// if -1 is send, something went wrong, dates, country or disease
			fprintf(stdout,"\033[1;34mRESULT: \033[0mAn error has occured\n\n");
			fflush(stdout);	
		}

	}else if(!strcmp(token,"/diseaseFrequency")){

		fprintf(stdout,"\033[1;36mREQUEST:  \033[0m%s\n",tempbuffer );
		fflush(stdout);
		tok = strtok(buff,s);	
		
		if(!strcmp(tok,"1")){
			tok = strtok(NULL,s);
			fprintf(stdout,"\033[1;34mRESULT:  \033[0m%s\n\n",tok);
			fflush(stdout);
		}else if(!strcmp(tok,"-1")){		// if -1 is send, something went wrong, dates, country or disease
			fprintf(stdout,"\033[1;34mRESULT: \033[0mAn error has occured\n\n");
			fflush(stdout);	
		}
	}else{
		err("Wrong Command");
	}

	funlockfile(stdout);   // unlock the stdout, so another thread can write

	/* When threads terminate, they decrement the active thread count */
	pthread_mutex_lock(&mtx);
	activeThreads--;
    pthread_cond_signal(&condWaiting); // signal to main thread, when a thread exits, make it recheck the waiting thread count if waiting for all threads to wait 
  	pthread_mutex_unlock(&mtx);
  	close(sock); 	/* Close socket and exit */
  	pthread_exit(NULL);
}

int main(int argc, char const *argv[]){

	char queryFile[32];
	int numThreads;
	FILE* fp;
	size_t len = 0;
	char buffer[256];
	int countThreads = 0;
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

	if((fp = fopen(queryFile,"r")) == NULL)	//open file
		err("Can not open file");

	int numOfLines=0;
	char temp[256];
	while(fgets(temp,256,fp)!=NULL){		// count the  lines of the file
		numOfLines++;
	}

	fseek(fp,0,SEEK_SET);			// go to the start of the file

	/*---------------------------- Create Threads -----------------------------------*/

	pthread_mutex_init(&mtx,NULL);
	pthread_cond_init(&cond,NULL);
	pthread_cond_init(&condWaiting,NULL); 

	activeThreads = waitingThreads = readyFlag = 0;
	char line[100];

	if((tids = malloc(numOfLines*sizeof(pthread_t))) == NULL)
		err("malloc");

	char** lines = malloc(numOfLines*sizeof(char*));

	for(int i=0 ; i < numOfLines ; i++){
		fscanf(fp,"%[^\n]\n",line);		// read a command from the file
		lines[i] = malloc((strlen(line)+1)*sizeof(char));
		lines[i][0]='\0';
		
		strcpy(lines[i],line);

		if(pthread_create(tids+i, NULL ,thread_function, (void*) lines[i])<0)		// create thread, give the command
			err("create thread");
		activeThreads++;
		countThreads++;

		if((i+1) == numThreads){		// when all the threads are created
			countThreads = 0;
			pthread_mutex_lock(&mtx);
		    
		    while (waitingThreads < activeThreads){ /* wait on 'condWaiting' until all active threads are waiting */
		      pthread_cond_wait(&condWaiting,&mtx);
		    }
		    if(waitingThreads != 0){
		      readyFlag = 1;
		      pthread_cond_broadcast(&cond);		// set free all the threads
		    }

		  	pthread_mutex_unlock(&mtx);

		  	pthread_mutex_lock(&mtx);
		    
		    if(waitingThreads == 0)
		      readyFlag = 0;
		  	
		  	pthread_mutex_unlock(&mtx);
		}
	}

	if (countThreads > 0){
		countThreads = 0;
		pthread_mutex_lock(&mtx);
	    
	    while (waitingThreads < activeThreads){ /* wait on 'condWaiting' until all active threads are waiting */
	      pthread_cond_wait(&condWaiting,&mtx);
	    }
	    if(waitingThreads != 0){
	      readyFlag = 1;
	      pthread_cond_broadcast(&cond);		// set free all the threads
	    }

	  	pthread_mutex_unlock(&mtx);

	  	pthread_mutex_lock(&mtx);
	    
	    if(waitingThreads == 0)
	      readyFlag = 0;
	  	
	  	pthread_mutex_unlock(&mtx);
	}		
	

	for(int i=0 ; i < numOfLines; i++){		/* Wait for thread termination */
		if(pthread_join(*(tids + i),NULL)<0) 
			err("pthread_join");
	}

	pthread_cond_destroy(&condWaiting);
	pthread_cond_destroy(&cond);
	pthread_mutex_destroy(&mtx);
	for (int i = 0; i < numOfLines; i++){
		free(lines[i]);
	}
	free(lines);
	free(tids);
	fclose(fp);
	return 0;
}
