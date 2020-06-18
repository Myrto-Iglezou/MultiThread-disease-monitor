#include "../include/utils.h"

void findRanges(statistics** stat,BucketRecord* record){
	searchTree(record->root,stat);
}

int findNum(int id,workerInfo ** array,int numWorkers){		// find the number of the worker from the pid
	for(int i=0; i<numWorkers ;i++){
		if(array[i]->pid == id)
			return i;
	}
}

int findWorkerFromCountry(char* country, workerInfo ** array,int numWorkers,int *counter){		// find the number of the worker from the country
	for(int i=0; i<numWorkers ;i++){
		for (int j = 0; j < counter[i]; j++){
			if(!strcmp(country,array[i]->countries[j]))
				return i;
		}
	}
	return -1;
}

void writeToSocket(int fd,char* message){
	int message_size = (int) strlen(message);
	write(fd,&message_size,sizeof(int));
	write(fd,message,strlen(message)+1);
}

void readFromSocket(int fd,char* buffer){
	int message_size;
	read(fd,&message_size,sizeof(int));
	read(fd,buffer,message_size+1);
}
void readBytes(int rfd,char* buffer,int bufferSize,int message_size){
	int num;
	char readBuffer[256];
	strcpy(buffer,"");

	if(bufferSize>message_size)
		bufferSize = message_size + 1;

	int countBytes=0;
	while(countBytes<message_size){			
		if((num = read(rfd,readBuffer,bufferSize))<0)
			err("Problem in reading!");
			
		strncat(buffer,readBuffer,num);
		
		countBytes = strlen(buffer);
	}
}

void writeBytes(char * data,int wfd, int bufferSize){
	char *strPointer = &data[0];
	int size = bufferSize;
	int count = 0;
	int message_size = strlen(data);
	char tempstr[256];

	if(bufferSize>message_size)
		size = message_size + 1;

	if(write(wfd,&message_size,sizeof(int))<0)
		err("Problem in writing");

	while(count < message_size){

		strPointer = &data[0];
		strPointer+=count;
		
		if(((strlen(data)+1)-count)<size){
			size = (strlen(data)+1)-count;					
		}
	
		strncpy(tempstr,strPointer,size);
		if(write(wfd,tempstr,size)<0)
			err("Problem in writing");

		count+=size;
	}
}

void sendStat(char* data,int bufferSize,int wfd){
	int count=0;
	char tempstr[256];
	char* strPointer = data;
	int size = bufferSize;

	int message_size = strlen(data);

	if(bufferSize>message_size)
		size = message_size + 1;

	if(write(wfd,&message_size,sizeof(int))<0)
		fprintf(stdout,"Problem in writing");

	while(count < message_size){
		
		if(((strlen(data)+1)-count)<size){
			size = (strlen(data)+1)-count;					
		}
		
		strncpy(tempstr,strPointer,size);
		if(write(wfd,tempstr,size)<0)
			err("Problem in writing");

		count+=size;
		strPointer+=size;
	}
}

void searchTree(Treenode* root,statistics** stat){		// find the ranges in the tree
	if(root==guard)
		return;
	searchTree(root->left,stat);
	searchTree(root->right,stat);
	Patient* pat = (Patient*) root->data;
	int age = pat->age;
	if(age>=0 && age<= 20)
		(*stat)->ranges[0]++;
	else if(age>=21 && age<= 40)
		(*stat)->ranges[1]++;
	else if(age>=41 && age<= 60)
		(*stat)->ranges[2]++;
	else if(age>=60)
		(*stat)->ranges[3]++;
}

void printStat(statistics * stat){
	fprintf(stdout,"\n%s\n",stat->date);
	fprintf(stdout,"%s\n",stat->country);
	fprintf(stdout,"%s\n",stat->disease );
	for (int i = 0; i < 4; i++){
		switch(i){
			case 0:
				fprintf(stdout,"Age range 0-20 years: ");
				break;
			case 1:
				fprintf(stdout,"Age range 21-40 years: ");	
				break;
			case 2:
				fprintf(stdout,"Age range 41-60 years: ");
				break;
			case 3: 
				fprintf(stdout,"Age range 60+ years: ");
		}
		fprintf(stdout,"%d cases\n",stat->ranges[i]);
	}
}

int CheckDate(char entryDate[strlen(DATE)+1],char exitDate[strlen(DATE)+1]){			// check if exit date is sooner than the entry date
	char temp[sizeof(DATE)];
	char day1[3],day2[3],month1[3],month2[3],year1[5],year2[5];

	const char s[2] = "-";
   	char *token;
   
   strcpy(temp,entryDate);
   token = strtok(temp, s);
   
   	if(!strcmp(exitDate,"–") || !strcmp(exitDate,"-")){		// if there's no exit date do not make the check;
   		return TRUE;
	}else{

	   	for(int i=0;i<3;i++){		// split the entry date
	    	
	    	if(i==0)
	    		strcpy(day1,token);
	    	if(i==1)
	    		strcpy(month1,token);
	    	if(i==2)
	    		strcpy(year1,token);
	   	
	      	token = strtok(NULL, s);
	    }

	   strcpy(temp,exitDate);
	   token = strtok(temp, s);
	   
	   for(int i=0;i<3;i++){		// split the exit date
	    	
	    	if(i==0)
	    		strcpy(day2,token);
	    	if(i==1)
	    		strcpy(month2,token);
	    	if(i==2)
	    		strcpy(year2,token);
	   	
	      	token = strtok(NULL, s);
	   }
	}
	if(atoi(month2)>12 || atoi(month1)>12)
		return FALSE;
	if(atoi(day2)>31 || atoi(day1)>31)
		return FALSE;
	if(atoi(year2)<atoi(year1))
		return FALSE;
	else if(atoi(year2)==atoi(year1) && atoi(month2)<atoi(month1))
		return FALSE;
	else if(atoi(year2)==atoi(year1) && atoi(month2)==atoi(month1) && atoi(day2)<atoi(day1))
		return FALSE;
	else return TRUE;
}
