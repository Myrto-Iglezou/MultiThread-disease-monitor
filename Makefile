# in order to execute this "Makefile" just type "make"

OBJS   = master.o worker.o patient.o RBT.o comparators.o hashtable.o utils.o dataFunctions.o heap.o whoServer.o whoClient.o
SOURCE = master.c worker.c patient.c RBT.c comparators.c hashtable.c utils.c dataFunctions.c heap.c whoServer.c whoClient.c
HEADER = patient.h RBT.h comparators.h hashtable.h utils.h dataFunctions.h heap.h
OUT    = master worker whoServer whoClient
CC     = gcc
FLAGS  = -g -c

all: master worker whoServer whoClient

master: master.o utils.o
	$(CC) -g master.o utils.o  -o master 

whoClient: whoClient.o utils.o
	$(CC) -g whoClient.o utils.o -o whoClient -lpthread

whoServer: whoServer.o utils.o
	$(CC) -g whoServer.o utils.o -o whoServer -lpthread

worker: worker.o patient.o RBT.o comparators.o dataFunctions.o hashtable.o utils.o heap.o
	$(CC) -g worker.o patient.o RBT.o comparators.o utils.o  dataFunctions.o hashtable.o heap.o -o worker -lm


whoClient.o: whoClient.c 
	$(CC) $(FLAGS) whoClient.c 

whoServer.o: whoServer.c 
	$(CC) $(FLAGS) whoServer.c 

master.o: master.c 
	$(CC) $(FLAGS) master.c 

worker.o: worker.c
	$(CC) $(FLAGS) worker.c 

utils.o: utils.c
	$(CC) $(FLAGS) utils.c 

patient.o: patient.c
	$(CC) $(FLAGS) patient.c

RBT.o: RBT.c
	$(CC) $(FLAGS) RBT.c

comparators.o: comparators.c
	$(CC) $(FLAGS) comparators.c

hashtable.o: hashtable.c
	$(CC) $(FLAGS) hashtable.c

dataFunctions.o: dataFunctions.c
	$(CC) $(FLAGS) dataFunctions.c

heap.o: heap.c
	$(CC) $(FLAGS) heap.c 

# clean house
clean:
	rm -f $(OBJS) $(OUT)

# do a bit of accounting
count:
	wc $(SOURCE) $(HEADER)
