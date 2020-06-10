#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char const *argv[]){

	char queryPortNum[32],statisticsPortNum[32];
	int numThreads,bufferSize;

	for(int i=0; i<argc;i++){
		
		if(!strcmp(argv[i],"-q"))
			strcpy(queryPortNum,argv[i+1]);
		if(!strcmp(argv[i],"-b"))
			bufferSize = atoi(argv[i+1]);
		if(!strcmp(argv[i],"-s"))
			strcpy(statisticsPortNum,argv[i+1]);
		if(!strcmp(argv[i],"-w"))
			numThreads =  atoi(argv[i+1]);
	}


	return 0;
}