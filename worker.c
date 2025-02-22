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
#include <sys/wait.h> /* sockets */
#include <sys/types.h> /* sockets */
#include <sys/socket.h> /* sockets */
#include <netinet/in.h> /* internet sockets */
#include <arpa/inet.h>
#include <poll.h>
#include <netdb.h> /* ge th os tb ya dd r */
#include "./include/utils.h"
#define TRUE 1
#define FALSE 0

static volatile sig_atomic_t SIGINTFlag = FALSE;

void SIGINTHandler(int sig_num){
	SIGINTFlag = TRUE;
}


int main(int argc, char const *argv[]){

	int rfd,wfd,bufferSize;
	char readFifo[32];
	int num,numOfstat=0;
	DIR * dir;
	FILE * fp;
	char* token;
	struct dirent * dir_info;
	int n=0,count=0;
	char path[100];
	char country[40],recordID[10],state[10],patientFirstName[30],patientLastName[30],disease[20],age[5];
	Patient* pat;
	guard = createGuard();
	Treenode * root = guard;
	Treenode * tempNode;
	char* date1 = malloc((strlen(DATE)+1)*sizeof(char));
	char* date2 = malloc((strlen(DATE)+1)*sizeof(char));
	char* buffer;
	int message_size;
	int countBytes=0;
	int numOfFolders=0;
	int maxFolders=0;
	char tempstr[256];
	HashTable* diseaseHashtable = NULL;
	HashTable* countryHashtable = NULL;
	HashTable* tableForStatistics = NULL;
	int numOfentries = 20;
	char* strPointer;
	char message[256];
	statistics * stat = calloc(1,sizeof(statistics));
	statistics ** arrayOfStat = malloc(sizeof(statistics*));
	int numOfFiles;
	char serverIP[32];
	int serverPort;
	
	/*--------------------------- Handle Signals -------------------------------------*/ 

	static struct sigaction SIGINTact;

	SIGINTact.sa_handler = SIGINTHandler;
	sigfillset(&(SIGINTact.sa_mask));

	if(sigaction(SIGINT,&SIGINTact,NULL) == -1)
		err("sigaction error");

	/*---------------------------- Read from the input -------------------------------*/

	for(int i=0; i<argc;i++){
		
		if(!strcmp(argv[i],"-rfn"))
			strcpy(readFifo,argv[i+1]);
		if(!strcmp(argv[i],"-b"))
			bufferSize = atoi(argv[i+1]);
	}

    while(1){

		rfd = open(readFifo, O_RDONLY );
		if(rfd > 0)
			break;
	}

	/*------------------------  Read IP adress - Port number ------------------------*/
	
	if(read(rfd,&message_size,sizeof(int))<0)	
			err("Problem in reading bytes");

	buffer = malloc((message_size+1)*sizeof(char));
	strcpy(buffer,"");

	readBytes(rfd,buffer,bufferSize,message_size);	
	serverPort = atoi(buffer);

	free(buffer);

	if(read(rfd,&message_size,sizeof(int))<0)	
			err("Problem in reading bytes");

	buffer = malloc((message_size+1)*sizeof(char));
	strcpy(buffer,"");

	readBytes(rfd,buffer,bufferSize,message_size);	
	strcpy(serverIP,buffer);

	free(buffer);

	/*----------------------------------------------------------------------------------*/
	
	if(read(rfd,&maxFolders,sizeof(int))<0)		// read the number of idrectories he has to read
		err("Problem in reading bytes");

	char* subdirectories[maxFolders]; 

	char* countries[maxFolders]; 

	char** filesPerDir[maxFolders];

	int numOfFilesPerDir[maxFolders];

	/*----------------------- Create the two hashtables -----------------------*/

	if(createHashTable(&diseaseHashtable,numOfentries)==FALSE){
		err("there's no space for a record in the disease hashtable\n");
		DeleteHashTable(diseaseHashtable);
		free(guard);
		return 0;
	}
	if(createHashTable(&countryHashtable, maxFolders)==FALSE){
		err("there's no space for a record in the country hashtable\n");
		DeleteHashTable(diseaseHashtable);
		DeleteHashTable(countryHashtable);
		free(guard);
		return 0;
	}

	/*----------------------- Read the subdirectories ---------------------------*/

	while(numOfFolders<maxFolders){

		if(read(rfd,&message_size,sizeof(int))<0)	
			err("Problem in reading bytes");

		buffer = malloc((message_size+1)*sizeof(char));
		strcpy(buffer,"");

		readBytes(rfd,buffer,bufferSize,message_size);		// read the path for the directory

		if((dir = opendir(buffer)) == NULL)	//open directory
			err("Can not open directory");

		subdirectories[numOfFolders] = malloc((strlen(buffer)+1)*sizeof(char));
		strcpy(subdirectories[numOfFolders],buffer);		// save the path in an array

		n=0;
		numOfFiles=0;

		while((dir_info = readdir(dir)) != NULL){		// count the files in the directory
			numOfFiles++;
		}

		numOfFilesPerDir[numOfFolders] = numOfFiles-2;		// remove 2 for . and ..

		filesPerDir[numOfFolders] = malloc(numOfFiles*sizeof(char*));

		char *namelist[numOfFiles];
		rewinddir(dir);		// move pointer in the start of the directory
		while((dir_info = readdir(dir)) != NULL){
			strcpy(path,buffer); 
			if(!strcmp(dir_info->d_name,".") || !strcmp(dir_info->d_name,".."))	continue;
			
			filesPerDir[numOfFolders][n] = malloc((strlen(DATE)+1)*sizeof(char));	 // save the files in an array of every country
			strcpy(filesPerDir[numOfFolders][n],dir_info->d_name);		// save the names of the files(dates) in an array to sort them and read them
			namelist[n] = malloc((strlen(DATE)+1)*sizeof(char)); 
			strcpy(namelist[n],dir_info->d_name);
			n++;
		}
		
		qsort(&namelist[0],n,sizeof(char*),CompareDates);	// sort the names of the files 

		for(int i=0 ; i<n ; i++){

			if(createHashTable(&tableForStatistics,numOfentries)==FALSE){		// create hash table for the diseases to help find statistics
				err("there's no space for a record in the disease hashtable\n");
				DeleteHashTable(tableForStatistics);
				free(guard);
				return 0;
			}
			strcpy(path,buffer); 
			strcat(path,"/");

			strcpy(tempstr,path);
			token = strtok(tempstr,"/");
			while(token !=NULL){
				strcpy(country,token);
				token = strtok(NULL,"/");
			}
			countries[numOfFolders] = malloc((strlen(country)+1)*sizeof(char));
			strcpy(countries[numOfFolders],country);		// save the country to an array
			strcat(path,namelist[i]);
			
			if((fp = fopen(path,"r"))== NULL)	//open file
				err("Can not open file");

			while(!feof(fp)){
				fscanf(fp,"%s %s %s %s %s %s \n",recordID,state,patientFirstName,patientLastName,disease,age);
				if(!strcmp(state,"ENTER")){
					strcpy(date1,namelist[i]);
					strcpy(date2,"-");
				}else if(!strcmp(state,"EXIT")){
					strcpy(date2,namelist[i]);
					strcpy(date1,"-");
				}
				
				pat =  createPatient(recordID,patientFirstName,patientLastName,disease,country,date1,date2,atoi(age));
				tempNode = FindData(root,pat,ComparePatientsID);
				Patient* temppat  = NULL;
				if(tempNode!=guard)
					temppat = (Patient*) tempNode->data;
				
				if(tempNode==guard && !strcmp(state,"ENTER")){		// if there isn't a patient with this id and this record's state is Enter add him to the rbt
					insertion(&root,pat,ComparePatientsID);			// insert patient to RBT
					insertToHashTable(diseaseHashtable,pat,pat->disease,ComparePatientsDates);
					insertToHashTable(countryHashtable,pat,pat->country,ComparePatientsDates);
					insertToHashTable(tableForStatistics,pat,pat->disease,CompareAges);
				}else if(tempNode!=guard && !strcmp(state,"EXIT") && !strcmp(temppat->exitDate,"-")){		// if there is a patient with this id with no exit date and this record's state is Exit, add him to the rbt
					deletePatient(pat);
					pat = tempNode->data;
					strcpy(date1,pat->entryDate);
					if(CompareDates(&date1,&date2)<=0)
						strcpy(pat->exitDate,date2);
				}else{					// in every other case print error
					deletePatient(pat);
				}
			}
			
			fclose(fp);
			strcpy(stat->date,namelist[i]);
			strcpy(stat->country,country);
			for(int i=0 ; i<4 ; i++){
				stat->ranges[i] = 0;
			}
			Bucket* curBucket;
			BucketRecord* record;
			count=0;

			for(int i=0; i<tableForStatistics->NumOfEntries; i++){			// for every disease in the hashtable
				curBucket = tableForStatistics->listOfBuckets[i];
				while(curBucket!=NULL){
					for(int j=0 ; j<curBucket->currentRecords ; j++){
						for(int i=0 ; i<4 ; i++)
							stat->ranges[i] = 0;
						count=0;
						record = curBucket->records[j];
						strcpy(stat->disease,record->data);
						findRanges(&stat,record);
						arrayOfStat = realloc(arrayOfStat,(numOfstat+1)*sizeof(statistics*));
						arrayOfStat[numOfstat] = calloc(1,sizeof(statistics));
						*arrayOfStat[numOfstat] = *stat;		// save all statistics in an array
						numOfstat++;
					}
					curBucket = curBucket->next;
				}
			}		
			DeleteHashTable(tableForStatistics);
		}
		for (int i = 0; i < n; i++){
			free(namelist[i]);
		}
		free(buffer);
		closedir(dir);
		numOfFolders++;
	}

	/*-------------------------- Connect to server -----------------------------------*/

	int sock;

	struct sockaddr_in server;
	struct sockaddr * serverptr = (struct sockaddr *) &server;
	struct hostent * rem;
	struct in_addr server_addr;

	inet_aton(serverIP,&server_addr);

	if((sock = socket(AF_INET, SOCK_STREAM,0)) < 0)			/* Create socket */
		err("Socket");

	if((rem = gethostbyaddr((const char*)&server_addr,sizeof(server_addr),AF_INET)) == NULL)		/* Find server address */
		err("gethostbyaddr");
	
	server.sin_family = AF_INET;		 /* Internet domain */
	memcpy(&server.sin_addr,rem->h_addr,rem->h_length);
	server.sin_port = htons(serverPort);	 /* Server port */

	if(connect(sock,serverptr,sizeof(server)) < 0)
		err("Connect");

	printf("Connect with server\n");

	/*---------------------- Recieve requests from server ------------------------------*/ 

	int queries_fd,server_fd;
	struct sockaddr_in worker_server;
	socklen_t workerlen = sizeof(struct sockaddr_in);
	struct sockaddr *worker_serverptr=(struct sockaddr *) &worker_server;

	if((queries_fd = socket(AF_INET , SOCK_STREAM ,0)) < 0)			/* Create socket */
		err("Socket");

	worker_server.sin_family = AF_INET; 		/* Internet domain */
	worker_server.sin_addr.s_addr = htonl(INADDR_ANY);
	worker_server.sin_port = 0; 		/* autoselect 0 */
	
	if(bind(queries_fd,worker_serverptr,sizeof(worker_server)) < 0)		/* Bind socket to address */
		err("Bind");
	
	if(getsockname(queries_fd,worker_serverptr,&workerlen)<0)
		err("getsockname");

	if(listen(queries_fd,SOMAXCONN) < 0)		/* Listen for connections */
		err("Listen");

	printf("My port is %d\n",ntohs(worker_server.sin_port));

	int tempport = ntohs(worker_server.sin_port);

	write(sock,&tempport,sizeof(int));
		
	/*--------------------------- Send Statistics -------------------------------------*/ 

	for (int i = 0; i < numOfstat; i++){	// send all statistics for every country to the parent
		if(write(sock,arrayOfStat[i],sizeof(statistics))<0)
			err("problem in writing");
	}
	
	close(sock);

	printf("End of sending statistics\n");
	
	/*---------------------------------------------------------------------------------*/

	int rc;
	struct pollfd *tempfd;
	tempfd = calloc(1,sizeof(struct pollfd));
	tempfd[0].fd = queries_fd;
	tempfd[0].events = POLLIN;
	char readbuffer[256];
	const char s[2] = " ";

	char diseaseCountry[64];
	char tempbuffer[32];


	printf("start poll\n");
	
	while(1){

		if(SIGINTFlag){
			SIGINTFlag = FALSE;
			break;	
		}
		rc = poll(tempfd,1,1);	
		if(rc > 0){
			if((tempfd[0].revents & POLLIN)){	
				if((server_fd = accept(queries_fd,NULL,0)) < 0)		/* accept connection */
					err("Accept");

				read(server_fd,readbuffer,sizeof(readbuffer));

				token = strtok(readbuffer,s);

				if(!strcmp(token,"/diseaseFrequency")){ 
					strcpy(country,"-");
					char tempbuffer[32];
					char readbuffer[150];

					for (int i = 0; i < 4; i++){		// reacieve all the info from the parent

						if(token!=NULL){
							token = strtok(NULL,s);

							if(i==0 && token!=NULL)
								strcpy(disease,token);
							else if(i==1 && token!=NULL)
								strcpy(date1,token);
							else if(i==2 && token!=NULL)
								strcpy(date2,token);
							else if(i==3 && token!=NULL)
								strcpy(country,token);
						}						
					}
					if(!CheckDate(date1,date2)){		// if dates are wrong
						strcpy(readbuffer,"-1");		// send server -1 (error)
						strcat(readbuffer,"$");
						write(server_fd,readbuffer,strlen(readbuffer)+1); 
					}else{	
						count = diseaseFrequency(country,disease,diseaseHashtable,countryHashtable,date1,date2,FALSE,FALSE);	// find the frequency
						sprintf(tempbuffer,"%d",count);
						if(count<0){					// if count <0 , means there is a problem, country or disease doesn't exist
							strcpy(readbuffer,"-1");	// send server -1 (error)
							strcat(readbuffer,"$");
							write(server_fd,readbuffer,strlen(readbuffer)+1); 
						}else{
							strcpy(readbuffer,"1");
							strcat(readbuffer,"$");
							strcat(readbuffer,tempbuffer);
							write(server_fd,readbuffer,strlen(readbuffer)+1); 	// send the result
						}	
					}

				}else if(!strcmp(token,"/topk-AgeRanges")){
					char k[5];
					heap* newheap;
					createHeap(&newheap);	// create the heap

					for (int i = 0; i < 5; i++){		// read the info

						if(token!=NULL){
							token = strtok(NULL,s);

							if(i==0 && token!=NULL)
								strcpy(k,token);
							else if(i==1 && token!=NULL)
								strcpy(country,token);
							else if(i==2 && token!=NULL)
								strcpy(disease,token);
							else if(i==3 && token!=NULL)
								strcpy(date1,token);
							else if(i==4 && token!=NULL)
								strcpy(date2,token);
						}	
					}

					int countRanges[4];
					int total = 0;
					float per = 0.0;
					int max;
					char data[10];
					char readbuffer[256];

					if(!CheckDate(date1,date2)){		// if dates are wrong
						strcpy(readbuffer,"-1");		// send server -1 (error)
						strcat(readbuffer,"$");
						write(server_fd,readbuffer,strlen(readbuffer)+1); 
					}else{	

						for (int i = 0; i < 4; i++){		// find the num of cases of every range
							countRanges[i] = findRange(country,disease,diseaseHashtable,countryHashtable,date1,date2,i);
							sprintf(tempbuffer,"%d",i);
							insertToHeap(countRanges[i],tempbuffer,newheap);
							total += countRanges[i] ;
						}

						if(total<0){					// if total < 0 , means there is a problem, country or disease doesn't exist
							strcpy(readbuffer,"-1");	// send server -1 (error)	
							strcat(readbuffer,"$");
							write(server_fd,readbuffer,strlen(readbuffer)+1); 
						}else{
							strcpy(readbuffer,"1");
							strcat(readbuffer,"$");

							if(atoi(k)>4)			// if the number requested is greater than the number of the ranges
								strcpy(k,"4");

							for(int i=0;i<atoi(k);i++){
								getTheMax(data,&max,newheap);		// find the max of the heap
								if(total!=0){
									per = (float) ((float) countRanges[atoi(data)] / (float) total);		// find the percentage
									sprintf(tempbuffer,"%lf",per);
								}else{
									per = 0.0;					// find the percentage
									sprintf(tempbuffer,"%lf",per);
								}

								strcat(readbuffer,data);
								strcat(readbuffer,"$");
								strcat(readbuffer,tempbuffer);
								strcat(readbuffer,"$");
							}
							write(server_fd,readbuffer,strlen(readbuffer)+1); 
						}
					}
					
					DeleteHeap(newheap,newheap->root);
					free(newheap);				
				}else if(!strcmp(token,"/searchPatientRecord")){					
					if(token!=NULL)
						token = strtok(NULL,s);

					strcpy(patientFirstName,"-");
					strcpy(patientLastName,"-");
					strcpy(disease,"-");
					strcpy(country,"-");
					strcpy(date1,"-");
					strcpy(date2,"-");
					strcpy(age,"0");
					pat =  createPatient(token,patientFirstName,patientLastName,disease,country,date1,date2,atoi(age));		// create a temporary patient to check the tree
					tempNode = FindData(root,pat,ComparePatientsID);	
					deletePatient(pat);		
					if(tempNode!=guard){		// if it found it send 1 and all the info of the record
						pat = (Patient*) tempNode->data;
						strcpy(readbuffer,"1");
						strcat(readbuffer,"$");
						strcat(readbuffer,token);
						strcat(readbuffer,"$");
						strcat(readbuffer,pat->firstName);
						strcat(readbuffer,"$");
						strcat(readbuffer,pat->lastName);
						strcat(readbuffer,"$");
						sprintf(tempbuffer,"%d",pat->age);
						strcat(readbuffer,tempbuffer);
						strcat(readbuffer,"$");
						strcat(readbuffer,pat->disease);
						strcat(readbuffer,"$");
						strcat(readbuffer,pat->country);
						strcat(readbuffer,"$");
						strcat(readbuffer,pat->entryDate);
						strcat(readbuffer,"$");
						strcat(readbuffer,pat->exitDate);
						strcat(readbuffer,"$");
						write(server_fd,readbuffer,strlen(readbuffer)+1); 
				
					}else{		// if i didn't find it send 0; 
						strcpy(readbuffer,"0");
						strcat(readbuffer,"$");
						write(server_fd,readbuffer,strlen(readbuffer)+1); 						
					}
				}else if(!strcmp(token,"/numPatientAdmissions")){ 
					strcpy(country,"-");
					char temp[150];
					for (int i = 0; i < 4; i++){
						
						if(token!=NULL){
							token = strtok(NULL,s);

							if(i==0 && token!=NULL)
								strcpy(disease,token);
							else if(i==1 && token!=NULL)
								strcpy(date1,token);
							else if(i==2 && token!=NULL)
								strcpy(date2,token);
							else if(i==3 && token!=NULL)
								strcpy(country,token);
						}	
					}
					char readbuffer[100];

					if(!CheckDate(date1,date2)){		// if dates are wrong
						strcpy(readbuffer,"-1");		// send server -1 (error)
						strcat(readbuffer,"$");
						write(server_fd,readbuffer,strlen(readbuffer)+1); 
					}else{	
						if(!strcmp(country,"-")){
							char readbuffer[100];
							int total = 0;
							strcpy(readbuffer,"");
							for (int i = 0; i < maxFolders; i++){
								strcat(readbuffer,countries[i]);
								strcat(readbuffer,"$");
								count = diseaseFrequency(countries[i],disease,diseaseHashtable,countryHashtable,date1,date2,TRUE,FALSE); // find the frequency of the admisions
								total += count;
							}
							if(total<0){						// if total < 0 , means disease doesn't exist
								strcpy(readbuffer,"-1");		// send server -1 (error)
								strcat(readbuffer,"$");
								write(server_fd,readbuffer,strlen(readbuffer)+1); 
							}else{
								sprintf(readbuffer,"%d",total);
								strcat(readbuffer,"$");
								write(server_fd,readbuffer,strlen(readbuffer)+1); 	// send the result
							}
						}else{
							count = diseaseFrequency(country,disease,diseaseHashtable,countryHashtable,date1,date2,TRUE,FALSE);
							sprintf(tempbuffer,"%d",count);
							if(count<0){					// if count < 0 , means there is a problem, country or disease doesn't exist
								strcpy(readbuffer,"-1");	// send server -1 (error)
								strcat(readbuffer,"$");
								write(server_fd,readbuffer,strlen(readbuffer)+1); 
							}else{
								strcpy(readbuffer,"@");
								strcat(readbuffer,"$");
								strcat(readbuffer,tempbuffer);
								write(server_fd,readbuffer,strlen(readbuffer)+1); 	// send the result
							}		
						}
					}
				}else if(!strcmp(token,"/numPatientDischarges")){ 
					strcpy(country,"-");
					char temp[150];
					for (int i = 0; i < 4; i++){
						
						if(token!=NULL){
							token = strtok(NULL,s);

							if(i==0 && token!=NULL)
								strcpy(disease,token);
							else if(i==1 && token!=NULL)
								strcpy(date1,token);
							else if(i==2 && token!=NULL)
								strcpy(date2,token);
							else if(i==3 && token!=NULL)
								strcpy(country,token);
						}	
					}
					char readbuffer[100];

					if(!CheckDate(date1,date2)){		// if dates are wrong
						strcpy(readbuffer,"-1");		// send server -1 (error)
						strcat(readbuffer,"$");
						write(server_fd,readbuffer,strlen(readbuffer)+1); 
					}else{	

						if(!strcmp(country,"-")){
							char readbuffer[100];
							strcpy(readbuffer,"");
							int total = 0;
							for (int i = 0; i < maxFolders; i++){
								strcat(readbuffer,countries[i]);
								strcat(readbuffer,"$");
								count = diseaseFrequency(countries[i],disease,diseaseHashtable,countryHashtable,date1,date2,FALSE,TRUE);  // find the frequency of the dicharges
								total += count;
							}
							if(total<0){						// if total < 0 , means disease doesn't exist
								strcpy(readbuffer,"-1");		// send server -1 (error)
								strcat(readbuffer,"$");
								write(server_fd,readbuffer,strlen(readbuffer)+1); 
							}else{
								sprintf(readbuffer,"%d",total);
								strcat(readbuffer,"$");
								write(server_fd,readbuffer,strlen(readbuffer)+1); 	// send the result
							}
						}else{					
							count = diseaseFrequency(country,disease,diseaseHashtable,countryHashtable,date1,date2,FALSE,TRUE);  // find the frequency of the dicharges
							sprintf(tempbuffer,"%d",count);

							if(count<0){						// if count < 0 , means there is a problem, country or disease doesn't exist
								strcpy(readbuffer,"-1");		// send server -1 (error)
								strcat(readbuffer,"$");
								write(server_fd,readbuffer,strlen(readbuffer)+1); 
							}else{
								strcpy(readbuffer,"@");
								strcat(readbuffer,"$");
								strcat(readbuffer,tempbuffer);
								write(server_fd,readbuffer,strlen(readbuffer)+1); 	// send the result
							}		
						}
					}
				}				
			}
			close(server_fd);
		}
	}

	for (int i = 0; i < numOfstat; i++){
		free(arrayOfStat[i]);
	}
	free(arrayOfStat);

	close(rfd);

	DeleteHashTable(diseaseHashtable);
	DeleteHashTable(countryHashtable);

	free(guard);
	free(stat);
	deleteTree(root,deletePatient);
	free(date1);free(date2);
	for (int i = 0; i < maxFolders; i++){
		free(subdirectories[i]);
		free(countries[i]);
	}

	for (int i = 0; i < maxFolders; ++i){
		for (int j = 0; j < n; j++){
			free(filesPerDir[i][j]);
		}
		free(filesPerDir[i]);
	}
	return 0;
}