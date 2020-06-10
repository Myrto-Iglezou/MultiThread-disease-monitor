# in order to execute this "Makefile" just type "make"

OBJS   = src/master.o src/worker.o src/patient.o src/RBT.o src/comparators.o src/hashtable.o src/utils.o src/dataFunctions.o src/heap.o src/whoServer.o src/whoClient.o
SOURCE = src/master.c src/worker.c src/patient.c src/RBT.c src/comparators.c src/hashtable.c src/utils.c src/dataFunctions.c src/heap.c src/whoServer.c src/whoClient.c
HEADER = patient.h RBT.h comparators.h hashtable.h utils.h dataFunctions.h heap.h
OUT    = master worker whoServer whoClient
CC     = gcc
FLAGS  = -g -c

all: master worker whoServer whoClient

master: src/master.o src/utils.o
	$(CC) -g src/master.o src/utils.o  -o master 

whoClient: src/whoClient.o 
	$(CC) -g src/whoClient.o -o whoClient 

whoServer: src/whoServer.o 
	$(CC) -g src/whoServer.o -o whoServer 

worker: src/worker.o src/patient.o src/RBT.o src/comparators.o src/dataFunctions.o src/hashtable.o src/utils.o src/heap.o
	$(CC) -g src/worker.o src/patient.o src/RBT.o src/comparators.o src/utils.o  src/dataFunctions.o src/hashtable.o src/heap.o -o worker -lm


whoClient.o: src/whoClient.c 
	$(CC) $(FLAGS) src/whoClient.c 

whoServer.o: src/whoServer.c 
	$(CC) $(FLAGS) src/whoServer.c 

master.o: src/master.c 
	$(CC) $(FLAGS) src/master.c 

worker.o: src/worker.c
	$(CC) $(FLAGS) src/worker.c 

utils.o: src/utils.c
	$(CC) $(FLAGS) src/utils.c 

patient.o: src/patient.c
	$(CC) $(FLAGS) src/patient.c

RBT.o: src/RBT.c
	$(CC) $(FLAGS) src/RBT.c

comparators.o: src/comparators.c
	$(CC) $(FLAGS) src/comparators.c

hashtable.o: src/hashtable.c
	$(CC) $(FLAGS) src/hashtable.c

dataFunctions.o: src/dataFunctions.c
	$(CC) $(FLAGS) src/dataFunctions.c

heap.o: src/heap.c
	$(CC) $(FLAGS) src/heap.c 

# clean house
clean:
	rm -f $(OBJS) $(OUT)

# do a bit of accounting
count:
	wc $(SOURCE) $(HEADER)
