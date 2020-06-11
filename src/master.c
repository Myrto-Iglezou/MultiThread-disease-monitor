#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <dirent.h>
#include <fcntl.h>
#include <poll.h>
#include "../include/utils.h"

static volatile sig_atomic_t signalPid = -1;

static volatile sig_atomic_t SIGCHLDFlag = FALSE;


void SIGCHLD_pid(int sig, siginfo_t *info, void* context){
	signalPid = info->si_pid;
	SIGCHLDFlag = TRUE;
}

int main(int argc, char const *argv[]){
	
	int numWorkers, bufferSize;
	char input_dir[30];
	pid_t pid;
	DIR * dir;
	FILE * fp;
	struct dirent * dir_info;
	int numOfdir=0;
	char pidfdr[10];
	char pidfdw[10];
	char fifoBuffer[10];
	int num;
	char readBuffer[256];
	char serverIP[32],serverPort[32];

	/*--------------------------- Handle Signals -------------------------------------*/ 

	static struct sigaction SIGCHLDact;
	
	SIGCHLDact.sa_flags = SA_SIGINFO;
	SIGCHLDact.sa_sigaction = SIGCHLD_pid;

	if(sigaction(SIGCHLD,&SIGCHLDact,NULL) == -1)
		err("sigaction error");
	

	/*---------------------------- Read from the input -------------------------------*/

	for(int i=0; i<argc;i++){
		if(!strcmp(argv[i],"-w"))
			numWorkers = atoi(argv[i+1]);
		if(!strcmp(argv[i],"-i"))
			strcpy(input_dir,argv[i+1]);
		if(!strcmp(argv[i],"-b"))
			bufferSize = atoi(argv[i+1]);
		if(!strcmp(argv[i],"-s"))
			strcpy(serverIP,argv[i+1]);
		if(!strcmp(argv[i],"-p"))
			strcpy(serverPort,argv[i+1]);
	}
	if(bufferSize<0 || numWorkers<0){
		err("Wrong input");
	}

	/*------------------------------- Create Workers -----------------------------------*/

	workerInfo* workerArray[numWorkers];	// create an array to save some information of every worker (countries, pid, fds, etc)
	for(int i=0; i<numWorkers ;i++){
		workerArray[i] = malloc(sizeof(workerInfo));
	}

	char rfifo[5];
	char wfifo[5];

	int workerWfd,workerRfd;

	for(int i=0; i<numWorkers ;i++){
		sprintf(rfifo,"%d",i);
		strcat(rfifo,"r");
		sprintf(wfifo,"%d",i);
		strcat(wfifo,"w");

        workerArray[i]->writeFifo = malloc((strlen(rfifo)+1)*sizeof(char));		// create fifo for parent to write and worker to read
		strcpy( workerArray[i]->writeFifo,rfifo);
		
		if(mkfifo(workerArray[i]->writeFifo ,0666) == -1)
			err("No fifo was created");
	}

	for(int i=0; i<numWorkers ;i++){		// create workers with fork and exec

		pid = fork();

        if(pid == -1){
           	err("fork failed" );
        }
        else if(pid == 0){
        	sprintf(fifoBuffer,"%d",bufferSize);
           	execlp("./worker","worker","-rfn",workerArray[i]->writeFifo,"-b",fifoBuffer,NULL);
        }
        else{
        	workerArray[i]->pid = pid;		// save the process id of every worker
        	workerArray[i]->num = i;		// save the number of every worker
        }  

	}

	for(int i=0; i<numWorkers ;i++){
		
		while(1){
			if((workerArray[i]->writeFd = open(workerArray[i]->writeFifo, O_WRONLY)) < 0)		// open fifo for write
				printf("Could not open fifo..");

			if(workerArray[i]->writeFd > 0)
				break;
		}
	}

	/*-------------------- Send workers IP adress - Port number -------------------------*/

	for(int i = 0; i < numWorkers; i++){	

		writeBytes(serverPort,workerArray[i]->writeFd,bufferSize);
		writeBytes(serverIP,workerArray[i]->writeFd,bufferSize);
	}
	
	/*----------------------- Distribute the subdirectories -----------------------------*/

	if((dir = opendir(input_dir)) == NULL)	//open directory
		err("Can not open directory");
	
	int w=0;
	char path[256];
	memset(path,0,bufferSize);
	char* strPointer;
	char tempStr[256];
	int size;
	int message_size;
	memset(tempStr,0,sizeof(tempStr));


	int count=0;
	char cwd[256];
	getcwd(cwd,sizeof(cwd));
	int numOffiles = 0;

	while((dir_info = readdir(dir)) != NULL){
		if(!strcmp(dir_info->d_name,".") || !strcmp(dir_info->d_name,".."))	continue;
		numOffiles++;
	}
	rewinddir(dir);
	int filesPerWorker = numOffiles/numWorkers;		// for every worker calculate the num of directories
	int extraFiles = numOffiles%numWorkers;			// find the ones left
	int totalFiles = 0;

	for(int i = 0; i < numWorkers; i++){		
		
		totalFiles = filesPerWorker;		
		
		if(extraFiles>i)		// some of them will get an extra one
			totalFiles+=1;

		workerArray[i]->countries = malloc(totalFiles*sizeof(char*));

		for (int j = 0; j < totalFiles; j++){
			workerArray[i]->countries[j] = malloc(sizeof(char));
		}

		if(write(workerArray[i]->writeFd,&totalFiles,sizeof(int))<0)		// send to worker the number of countries he will have
			err("Problem in writing");
	}

	int countriesCounter[numWorkers];		// counter for parent to know the number of countries of evey worker
	for (int i = 0; i < numWorkers; i++){
		countriesCounter[i] = 0;
	}
	while((dir_info=readdir(dir)) != NULL){

		if(!strcmp(dir_info->d_name,".") || !strcmp(dir_info->d_name,".."))	continue;

		count=0;

		if(w == numWorkers)
			w=0;

		workerArray[w]->countries[countriesCounter[w]] = realloc(workerArray[w]->countries[countriesCounter[w]], (strlen(dir_info->d_name)+1)*sizeof(char));	// save the countries names 
		strcpy(workerArray[w]->countries[countriesCounter[w]],dir_info->d_name);

		countriesCounter[w]++;

		strcpy(path,cwd);
		strcat(path,"/");
		strcat(path,input_dir); 
		strcat(path,"/");
		strcat(path,dir_info->d_name);

		writeBytes(path,workerArray[w]->writeFd,bufferSize);	// send the absolute path for every directory to the worker
		w++;     
	}

	/*----------------------------------------------------------------------------------*/

	int flag = TRUE;
	
	while(flag){

		if(SIGCHLDFlag){	// if a child is killed

			int workerNum,id;
			id = signalPid;

			workerNum  = findNum(id,workerArray,numWorkers);
			
			pid = fork();		// create a new child

	        if(pid == -1){
	           	err("fork failed" );
	        }
	        else if(pid == 0){
		        sprintf(fifoBuffer,"%d",bufferSize);
	           	execlp("./worker","worker","-rfn",workerArray[workerNum]->writeFifo,"-s",serverIP,"-p",serverPort,"-b",fifoBuffer,NULL);
	        }
	        else{
	        	workerArray[workerNum]->pid = pid;
	        } 

	        close(workerArray[workerNum]->writeFd);

	        while(1){

				workerArray[workerNum]->writeFd = open(workerArray[workerNum]->writeFifo, O_WRONLY);	// open fifo for write
				
				if(workerArray[workerNum]->writeFd > 0)
					break;
			}

			if(write(workerArray[workerNum]->writeFd,&countriesCounter[workerNum],sizeof(int))<0)
				err("Problem in writing");

	        for (int i = 0; i < countriesCounter[workerNum]; i++){
	       		
	       		strcpy(path,cwd);
				strcat(path,"/");
				strcat(path,input_dir); 
				strcat(path,"/");
				strcat(path,workerArray[workerNum]->countries[i]);

				strPointer = &path[0];
				size = bufferSize;
				message_size = strlen(path);
				if(write(workerArray[workerNum]->writeFd,&message_size,sizeof(int))<0)
					err("Problem in writing");
				count = 0;
				while(count < strlen(path)){

					strPointer = &path[0];
					strPointer+=count;
					
					if(((strlen(path)+1)-count)<size){
						size = (strlen(path)+1)-count;					
					}
					strncpy(tempStr,strPointer,size);
					if(write(workerArray[workerNum]->writeFd,tempStr,size)<0)
						err("Problem in writing");
					count+=size;
				}  	
	        }   	
			
			SIGCHLDFlag = FALSE;
			signalPid = -1;
		}
		if(wait(NULL)<=0)
			flag = FALSE;
	}
		
	//while(wait(NULL)>0);

	/*--------------------------- Clean the memory -------------------------------------*/ 

	for(int i=0; i<numWorkers ;i++){
		close(workerArray[i]->writeFd);
		unlink(workerArray[i]->writeFifo);
	}

	closedir(dir);
	for(int i=0; i<numWorkers ;i++){

		for (int j = 0; j < countriesCounter[i] ; j++){
			free(workerArray[i]->countries[j]);
		}

		free(workerArray[i]->countries); 
		free(workerArray[i]->writeFifo);
		free(workerArray[i]); 
	}
	return 0;
}