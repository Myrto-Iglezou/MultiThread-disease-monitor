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
#include <errno.h>
#include <arpa/inet.h>
#include "./include/utils.h"
#define err(mess){fprintf(stderr,"\033[1;31mERROR: \033[0m: %s\n",mess);exit(1);}

typedef struct{			// struct in the buffer
	int fd;
	char info[5];
	char IP[32];
}fd_info;

typedef struct {		// struct for buffer
	fd_info* data;
	int start;
	int end;
	int count;
}buffer_t;

typedef struct {		// struct to keep information for the workers
	int port;
	char IP[32];
}worker_info;

buffer_t buffer;

worker_info** workersArray;
int numOfWorkers = 0;

int portExists(worker_info** array,int num,int port){		// check if the port in the array of workers exist
	for (int i = 0; i < num; i++){
		if(array[i]->port == port)
			return TRUE;
	}
	return FALSE;
}

pthread_mutex_t mtx;
pthread_mutex_t print_mtx;
pthread_cond_t cond_nonempty;
pthread_cond_t cond_nonfull;

void initialize(buffer_t * buffer,int num){			// inisialize the buffer
	buffer->start = 0;
	buffer->end = -1;
	buffer->count = 0;
	buffer->data = malloc(num*sizeof(fd_info));
}

void deleteBuffer(buffer_t* buffer){
	free(buffer->data);
}

void addFd(buffer_t* buffer,int fd,char* info,char* IP,int bufferSize){		// add a new fd in the buffer
	pthread_mutex_lock(&mtx);
	while(buffer->count >=bufferSize){
		pthread_cond_wait(&cond_nonfull, &mtx);
	}
	buffer->end = (buffer->end+1)%bufferSize;
	buffer->data[buffer->end].fd = fd;
	strcpy(buffer->data[buffer->end].info, info);
	strcpy(buffer->data[buffer->end].IP, IP);
	(buffer->count)++;
	pthread_mutex_unlock(&mtx);
}

fd_info* returnFdInfo(buffer_t* buffer,int bufferSize){			// reamove a fd from the buffer
	fd_info* data = malloc(sizeof(fd_info));
	pthread_mutex_lock(&mtx);
	while(buffer->count <= 0){
		pthread_cond_wait(&cond_nonempty, &mtx);
	}
	data->fd = buffer->data[buffer->start].fd;
	strcpy(data->info,buffer->data[buffer->start].info);
	strcpy(data->IP,buffer->data[buffer->start].IP);
	buffer->start = (buffer->start+1)%bufferSize;
	(buffer->count)--;
	pthread_mutex_unlock(&mtx);
	return data;
}

void * thread_function(void * arg){

	while(1){
		fd_info* data;
		int bufferSize = *(int*) arg; 

		data = returnFdInfo(&buffer,bufferSize);
		pthread_cond_signal(&cond_nonfull);

		if(!strcmp(data->info,"q")){				// if a message has arrived in the port for queries
			char buff[150];
			char readbuff[150];
			int socket_array[numOfWorkers];			// array to keep the fds of workers
			char* command;

		/*----------------------- Create connection with every worker ------------------------*/

			for (int i = 0; i < numOfWorkers; i++){	
				int sock;
				struct hostent *rem;
				struct sockaddr_in worker_server;
				struct sockaddr * worker_serverptr = (struct sockaddr *) &worker_server;
				struct in_addr worker_server_addr;

				inet_aton(workersArray[i]->IP,&worker_server_addr);

				if((sock = socket(AF_INET, SOCK_STREAM,0)) < 0)			/* Create socket */
					err("Socket");

				if((rem = gethostbyaddr((const char*)&worker_server_addr,sizeof(worker_server_addr),AF_INET)) == NULL)		/* Find worker_server address */
					err("gethostbyaddr");
				
				worker_server.sin_family = AF_INET;			 /* Internet domain */
				memcpy(&worker_server.sin_addr,rem->h_addr,rem->h_length);
				worker_server.sin_port = htons(workersArray[i]->port);	 /* worker_Server port */

				if(connect(sock,worker_serverptr,sizeof(worker_server)) < 0){
					if(errno == ECONNREFUSED){
						pthread_mutex_lock(&print_mtx);
						worker_info* temp;
						temp = workersArray[i];
						workersArray[i] = workersArray[numOfWorkers-1];			// if not save the info for the new worker
						free(temp);	
						numOfWorkers--;				// increase num of workers
						i--;
						pthread_mutex_unlock(&print_mtx);
						continue;

					}else
						err("Connect");
				}

				socket_array[i] = sock;
			}
		/*-------------------------------------------------------------------------------------*/
			char commandbuffer[100];
		
			if(read(data->fd,buff,sizeof(buff))<0)		// read the query from the client
				err("read");

			strcpy(commandbuffer,buff);

			command = strtok(commandbuffer," ");			// find the command

			char k[10];
			char* t = strtok(NULL," ");
			strcpy(k,t);


			char* tok;
			int flag;

			for (int i = 0; i < numOfWorkers; i++){			// write the query to every worker 
				if(write(socket_array[i],buff,strlen(buff)+1)<0)
					err("write");
			}

			flockfile(stdout);			// lock the stdout, so one thread can write
			fprintf(stdout,"\033[1;38mREQUEST:  \033[0m%s\n",buff );
			fflush(stdout);
			
			if(!strcmp(command,"/diseaseFrequency")){
				int total=0;
				flag = FALSE;
				char temp[50];
				for (int i = 0; i < numOfWorkers; i++){
					if(read(socket_array[i],readbuff,sizeof(readbuff))<0)
						err("read");
					strcpy(temp,readbuff);
					tok = strtok(temp,"$");
					if(!strcmp(tok,"1")){
						flag = TRUE;
						tok = strtok(NULL,"$");
						total+=atoi(tok);		// add result of every worker			
					}				
				}
				if(!flag){		// if none of the workers send a result > 0
					char readbuff[10];
					strcpy(readbuff,"-1");
					strcat(readbuff,"$");		// send -1 to client, in order he knows something went wrong
					if(write(data->fd,readbuff,strlen(readbuff)+1)<0)
						err("write");
					fprintf(stdout,"\033[1;32mRESULT:  \033[0m Something was wrong with query format\n\n");
					fflush(stdout);
				}else{
					char readbuff[150];
					char temp[20];
					strcpy(readbuff,"1");		// send 1 if there is a result
					strcat(readbuff,"$");
					sprintf(temp,"%d",total);
					strcat(readbuff,temp);
					strcat(readbuff,"$");
					if(write(data->fd,readbuff,strlen(readbuff)+1)<0)
						err("write");
					fprintf(stdout,"\033[1;32mRESULT:  \033[0m %s\n\n",temp);
					fflush(stdout);
				}					
			}else if(!strcmp(command,"/topk-AgeRanges")){
				flag = FALSE;
				char tempbuffer[150];
				char writebuffer[150];
				
				for (int i = 0; i < numOfWorkers; i++){
					char readbuffer[150];
					if(read(socket_array[i],readbuff,sizeof(readbuff))<0)
						err("read");
					strcpy(tempbuffer,readbuff);
					tok = strtok(tempbuffer,"$");
					if(!strcmp(tok,"1")){
						flag = TRUE;
						if(write(data->fd,readbuff,strlen(readbuff)+1)<0)
							err("write");
						strcpy(writebuffer,readbuff);
						break;
					}
				}
				if(!flag){		// if none of the workers found ranges
					char readbuff[10];
					strcpy(readbuff,"-1");		// send -1 to client, in order he knows something went wrong
					strcat(readbuff,"$");
					if(write(data->fd,readbuff,strlen(readbuff)+1)<0)
						err("write");
					fprintf(stdout,"\033[1;32mRESULT:  \033[0m Something was wrong with query format\n\n");
				}else{
					char* token,*tok;	
					if(atoi(k)>4)			// if the number requested is greater than the number of the ranges
						strcpy(k,"4");
					tok = strtok(writebuffer,"$");
					fprintf(stdout,"\033[1;32mRESULT:  \033[0m\n");
					fflush(stdout);

					for(int i=0 ; i<2*atoi(k) ; i++){
						tok = strtok(NULL,"$");
						if( tok!=NULL && !strcmp(tok,"0"))
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
			}else if(!strcmp(command,"/searchPatientRecord")){
				flag = FALSE;
				char temp[150];
				char readbuff[100];
				for (int i = 0; i < numOfWorkers; i++){
					if(read(socket_array[i],readbuff,sizeof(readbuff))<0)
						err("read");
					strcpy(temp,readbuff);
					tok = strtok(temp,"$");
					if(!strcmp(tok,"1")){
						flag = TRUE;			// if a worker found the record send the info to client
						if(write(data->fd,readbuff,strlen(readbuff)+1)<0)
							err("write");
					}
				}
				if(!flag){		// if no worker found the record id
					char readbuff[10];
					strcpy(readbuff,"0");		// send 0 to client, so he understands there is no record with that id
					strcat(readbuff,"$");
					if(write(data->fd,readbuff,strlen(readbuff)+1)<0)
						err("write");
					fprintf(stdout,"\033[1;32mRESULT: \033[0m No record with this id\n\n");
					fflush(stdout);	
				}else{
					fprintf(stdout,"\033[1;32mRESULT:  \033[0m");	
					fflush(stdout);	
					char *tok = strtok(readbuff,"$");
					tok = strtok(NULL,"$");
					while(tok!=NULL){
						fprintf(stdout,"%s ",tok);
						fflush(stdout);	
						tok = strtok(NULL,"$");
					}
					fprintf(stdout,"\n\n");
					fflush(stdout);
				}
					
			}else if(!strcmp(command,"/numPatientAdmissions") || !strcmp(command,"/numPatientDischarges")){
				flag = FALSE;
				char temp[150];
				char writebuffer[512];
				char readbuff[512];
				strcpy(writebuffer,"");
				int total = 0;
				int secFlag = FALSE;

				for (int i = 0; i < numOfWorkers; i++){		
					char readbuff[512];		
					if(read(socket_array[i],readbuff,sizeof(readbuff))<0)
						err("read");
					strcpy(temp,readbuff);
					tok = strtok(temp,"$");
					if(strcmp(tok,"-1") && strcmp(tok,"@")){	// if no specific country is given or there wasn't an error
						flag=TRUE;
						total += atoi(tok);		// add the results of all workers				
						
					}else if(!strcmp(tok,"@")){		// if a specific country is given, we expect result from one worker
						flag = TRUE;
						secFlag = TRUE;
						if(write(data->fd,readbuff,strlen(readbuff)+1)<0)
							err("write");	
						tok = strtok(readbuff,"$");
						tok = strtok(NULL,"$");
						fprintf(stdout,"\033[1;32mRESULT:  \033[0m%s\n\n",tok);	// we expect result for every country
						fflush(stdout);	
						break;					
					}				
				}
				if(!flag){		// if flag is FALSE, an error has occured
					char readbuff[10];
					strcpy(readbuff,"-1");		// send -1 to client, so he knows that
					strcat(readbuff,"$");
					if(write(data->fd,readbuff,strlen(readbuff)+1)<0)
						err("write");
					fprintf(stdout,"\033[1;32mRESULT: \033[0m Something was wrong with query format\n\n");
					fflush(stdout);	
				}else if(flag && !secFlag){			// if flag is TRUE send him the result
					sprintf(writebuffer,"%d",total);
					strcat(writebuffer,"$");
					if(write(data->fd,writebuffer,strlen(writebuffer)+1)<0)
						err("write");

					fprintf(stdout,"\033[1;32mRESULT:  \033[0m%d\n\n",total);	// we expect result for every country
					fflush(stdout);	
				}
			}else{
				char readbuff[10];
				strcpy(readbuff,"-1");		// send -1 to client, in order he knows something went wrong
				strcat(readbuff,"$");
				if(write(data->fd,readbuff,strlen(readbuff)+1)<0)
					err("write");
			}
			close(data->fd);
			for (int i = 0; i < numOfWorkers; i++){
				close(socket_array[i]);
			}
			funlockfile(stdout);   // unlock the stdout, so another thread can write
			free(data);
		}else if(!strcmp(data->info,"s")){		// if a message has arrived in the port of statistics
			/*  Receive Statistics */ 
			statistics* stat = calloc(1,sizeof(statistics));
			int port;

			read(data->fd,&port,sizeof(int));		// read the query port from worker

			if(!portExists(workersArray,numOfWorkers,port)){		// chech if this port already exists in the array of workers
				pthread_mutex_lock(&print_mtx);
				workersArray = realloc(workersArray,(numOfWorkers+1)*sizeof(worker_info*));
				workersArray[numOfWorkers] = malloc(sizeof(worker_info));
				workersArray[numOfWorkers]->port = port;			// if not save the info for the new worker
				strcpy(workersArray[numOfWorkers]->IP,data->IP); 
				numOfWorkers++;				// increase num of workers
				pthread_mutex_unlock(&print_mtx);
			}
			
			flockfile(stdout);
			while(read(data->fd,stat,sizeof(statistics))>0){		// read statistics
				// printStat(stat);		// print staistics
			}
			fprintf(stdout,"Receive Statistics\n");		
			funlockfile(stdout);
			free(stat);
			close(data->fd);
			free(data);
		}else
			err("problem with fd info");	
	}
	pthread_exit(NULL);
}

int main(int argc, char const *argv[]){

	int queryPortNum,statisticsPortNum;
	int numThreads,bufferSize;
	int query_fd,statistics_fd, answer_fd;
	struct sockaddr_in query_server, statistics_server,worker;

	struct sockaddr *query_serverptr=(struct sockaddr *) &query_server;
	struct sockaddr *statistics_serverptr=(struct sockaddr *) &statistics_server;
	struct sockaddr *workerptr=(struct sockaddr *) &worker;

	socklen_t workerlen = 0;
	char buff[64];
	int err;
	pthread_t * tids;
	workersArray = malloc(sizeof(worker_info*));

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
		
	if(listen(query_fd,SOMAXCONN) < 0)		/* Listen for connections */
		err("Listen");
	

	/*---------------------- Create socket for statistics -------------------------*/

	if((statistics_fd = socket(AF_INET , SOCK_STREAM ,0)) < 0)			/* Create socket */
		err("Socket");

	statistics_server.sin_family = AF_INET; 		/* Internet domain */
	statistics_server.sin_addr.s_addr = htonl(INADDR_ANY);
	statistics_server.sin_port = htons(statisticsPortNum); 		/* The given port */
	
	if(bind(statistics_fd,statistics_serverptr,sizeof(statistics_server)) < 0)		/* Bind socket to address */
		err("Bind");
		
	if(listen(statistics_fd,SOMAXCONN) < 0)		/* Listen for connections */
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
	int worker_fd;
	char IP[32];

	while(1){
		rc = poll(socket_fds,2,-1);	
		if(rc > 0){
			if((socket_fds[0].revents & POLLIN)){				// if a queryPort has request			
				if((answer_fd = accept(query_fd,NULL,0)) < 0)	/* accept connection */
					err("Accept");
				addFd(&buffer,answer_fd,"q","-",bufferSize);
				pthread_cond_signal(&cond_nonempty);

			}else if((socket_fds[1].revents & POLLIN)){			// if statistics have arrived
				if((answer_fd = accept(statistics_fd,workerptr,&workerlen)) < 0)	/* accept connection */
					err("Accept");

				printf("accept connection with worker\n");

				socklen_t len = sizeof(worker);	
				if(getpeername(answer_fd,(struct sockaddr*)&worker,&len)<0)		// find the IP of worker
					err("getpeername");

				strcpy(IP ,inet_ntoa(worker.sin_addr));

				addFd(&buffer,answer_fd,"s",IP,bufferSize);
				pthread_cond_signal(&cond_nonempty);
			}
		}
	}

	for(int i=0 ; i < numThreads ; i++){		/* Wait for thread termination */
		if((err=pthread_join(*(tids + i),NULL))<0) 
			err("pthread_join");

	}
	for (int i = 0; i < numOfWorkers; i++){
		free(workersArray[i]);
	}
	free(workersArray);
	close(query_fd);
	close(statistics_fd);
	free(tids);
	pthread_cond_destroy(&cond_nonempty);
	pthread_cond_destroy(&cond_nonfull);
	pthread_mutex_destroy(&mtx);

	return 0;
}