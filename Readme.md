
# System-Programming-project3

__Compile__: ```make``` <br>
__Execution for master__: ```./master -w 4 -b 256 -s 127.0.0.1 -p 3004 -i testDir``` <br>
__Execution for server__: ```./whoServer -w 10 -b 50 -q 3002 -s 3004``` <br>
__Execution for client__: ```./whoClient -q queries.txt -w 10 -sp 3002 -sip 127.0.0.1``` <br>

## Project Description

This project requires the implementation three programs:
1. a master program that creates a series of Worker processes.
2. a multi-threaded whoServer that collects summary statistics from the Worker processes through a network and queries from clients.
3. a multithreaded client whoClient program that generates multiple threads, where each thread plays the role of a client that sends queries to whoServer.

Τhese programs communicate with each other through sockets where needed.

![](/images/syspro3.PNG?raw=true "Communication of programs")
 

### A. The master program

``` ./master –w numWorkers -b bufferSize –s serverIP –p serverPort -i input_dir ```

<p> In the beginning, the master program starts certain number of Workers ( *numWorkers* ) child processes and distributes the subdirectories evenly with the countries in *input_dir* to the Workers. Then it has to inform each Worker via named pipe about the subdirectories that the Worker will undertake.The parent process sends via named pipe both the IP address and the port number of whoServer. When the creation of Worker processes is finished,  parent process remains to fork a new Worker process in case an existing Worker suddenly terminates. </p>

<p> Each Worker process, for each directory assigned to it, reads all its files in chronological order based on the file names and fill in a series of data structures that it will use to answer questions posed to it by whoServer. Then, it connects to whoServer and send it the following information: <br>
1. a port number where the Worker process will listen for questions that will be forwarded by whoServer, and
2. the summary statistics. <br>
When the Worker process finishes transferring information to whoServer, it will listen to the port number it has selected and wait for connections from whoServer for requests concerning the countries it manages.</p>

### B. The whoServer program

``` ./whoServer –q queryPortNum -s statisticsPortNum –w numThreads –b bufferSize ```

<p> When whoServer starts, the original thread creates a certain number of threads (*numThreads*). The main (main process) thread listens to the *queryPortNum* and *statisticsPortNum* ports, accepts connections to the accept() system call, and places the file/socket descriptors corresponding to the connections in a circular buffer of size defined by bufferSize. The original thread does NOT read from the links it receives. Simply, whenever it accepts a connection it places the file descriptor returned by accept() in the buffer, and continues to accept subsequent connections. The job of *numThreads* threads is to serve the connections whose corresponding file descriptors are placed in the buffer. Each of the *numThreads* threads wakes up when there is at least one descriptor in the buffer. </p>

WhoServer acceptσ and serveσ the following requests coming from whoClient:
* ``` /diseaseFrequency virusName date1 date2 [country] ```
If no country argument is given, whoServer will find for *virusName* the number of cases recorded in the system within [date1 ... date2]. If a country argument is given, whoServer will find for the virusName disease, the number of cases in the country that have been recorded in the period [date1 ... date2].
* ``` /topk-AgeRanges k country disease date1 date2 ```
WhoServer will find, for the country and the virus disease the top k age categories that have shown cases of the specific virus in the specific country and their incidence rate
* ``` /searchPatientRecord recordID ```
WhoServer forwards all Workers request and awaits response from Worker with record *recordID*.
* ``` /numPatientAdmissions disease date1 date2 [country] ```
If a country argument is given, whoServer should forward the request to the workers in order to find the total number of patients admitted to the hospital with the disease in that country within [date1 date2]. If no country argument is given, it will find the number of patients with the disease who were hospitalized in space [date1, date2].
* ``` /numPatientDischarges disease date1 date2 [country] ```
Given the country argument, whoServer will find the total number of patients with the disease who have been discharged from a hospital in that country within [date1, date2]. If no country argument is given, whoServer will find the number of patients with the disease who have been discharged from the hospital in space [date1, date2].

<p> When the whoServer accepts a query, it forwards it to the respective worker processes through a socket and waits for the answer from the workers. The query he forwarded to a Worker process along with the answers that whoServer receives from this Worker, prints them to stdout. WhoServer also forwards the answer to the corresponding whoClient thread that asked the query.</p>

### C. The whoClient program

``` ./whoClient –q queryFile -w numThreads –sp servPort –sip servIP ```

<p> The whoClient program starts and opens the queryFile file, and then reads it line by line. On each line there is one command that the whoServer can accept. For each command it creates a thread which will send the command (in the line) to whoServer. The thread is created, but it does not connect immediately to the whoServer. When all the threads are created, ie we have one thread for each command in the file, then the threads should start all together to try to connect to whoServer and send their command. When the command is sent, each thread prints the response received from whoServer to stdout and may end. When all threads are terminated, the whoClient terminates as well. </p>

### Implemantation techniques

- Communication between master and workers is done through named pipes.
- For workers, records are stored in a red black tree, which has the complexity of searching and entering, both in the middle and in the worst case O (logn).
- Handling was used for the SIGINT and SIGCHLD signals in the master, in order to clear the memory in the case of the first and to create a new worker in the case of the second.
-The Worker, after calculating the results, sends them to the server in a message, which has smaller messages separated by the character "$". If there is a "-1" at the beginning of the message, then the server realizes that what it requested could not be found. He communicates with the client in the same way.

