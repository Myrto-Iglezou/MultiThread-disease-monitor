# in order to execute this "Makefile" just type "make"

OBJS   = diseaseAggregator.o worker.o patient.o RBT.o comparators.o hashtable.o utils.o dataFunctions.o heap.o
SOURCE = diseaseAggregator.c worker.c patient.c RBT.c comparators.c hashtable.c utils.c dataFunctions.c heap.c
HEADER = patient.h RBT.h comparators.h hashtable.h utils.h dataFunctions.h heap.h
OUT    = diseaseAggregator worker
CC     = gcc
FLAGS  = -g -c

all: diseaseAggregator worker

diseaseAggregator: src/diseaseAggregator.o src/utils.o
	$(CC) -g src/diseaseAggregator.o src/utils.o  -o diseaseAggregator 

# create/compile the individual files >>seperetaly<<

worker: src/worker.o src/patient.o src/RBT.o src/comparators.o src/dataFunctions.o src/hashtable.o src/utils.o src/heap.o
	$(CC) -g src/worker.o src/patient.o src/RBT.o src/comparators.o src/utils.o  src/dataFunctions.o src/hashtable.o src/heap.o -o worker -lm


diseaseAggregator.o: src/diseaseAggregator.c 
	$(CC) $(FLAGS) src/diseaseAggregator.c 

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
