#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define err(mess){fprintf(stderr,"\033[1;31mERROR: \033[0m: %s\n",mess);exit(1);}

int main(int argc, char const *argv[]){

	char queryFile[32],servIP[32];
	int numThreads,servPort;
	FILE* fp;
	char* line = NULL;
	size_t len = 0;

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

	while(!feof(fp)){
		getline(&line,&len,fp);
		
	}
	
	return 0;
}