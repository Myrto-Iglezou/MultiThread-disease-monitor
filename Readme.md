
# System-Programming-project3

__Compile__: ```make```
__Execution for master__: ```./master -w 4 -b 256 -s 127.0.0.1 -p 3004 -i testDir``` <br>
__Execution for server__: ```./whoServer -w 10 -b 50 -q 3002 -s 3004``` <br>
__Execution for client__: ```./whoClient -q queries.txt -w 10 -sp 3002 -sip 127.0.0.1``` <br>

## Project Description

This project requires the implementation three programs:
1. a master program that creates a series of Worker processes.
2. a multi-threaded whoServer that collects summary statistics from the Worker processes through a network and queries from clients.
3. a multithreaded client whoClient program that generates multiple threads, where each thread plays the role of a client that sends queries to whoServer.

Τhese programs communicate with each other through sockets where needed.

### A. The master program

``` ./master –w numWorkers -b bufferSize –s serverIP –p serverPort -i input_dir ```

<p> In the beginning, the master program starts certain number of Workers ( *numWorkers* ) child processes and distributes the subdirectories evenly with the countries in *input_dir* to the Workers. Then it has to inform each Worker via named pipe about the subdirectories that the Worker will undertake.The parent process sends via named pipe both the IP address and the port number of whoServer. When the creation of Worker processes is finished,  parent process remains to fork a new Worker process in case an existing Worker suddenly terminates. </p>
Σχεδιαστικές επιλογές:

-Ολα τα ερωτήματα υπολογίζονται στους workers και τα αποτελέσματα στέλνονται στον
server, ο οποιίος με τη σειρά του τα εκτυπώνει και τα στέλνει στον client.

-Για την επικοινωνία ανάμεσα σε master και workers γίνεται μέσω named pipe, όπως
στην δεύτερη εργασια.

-Στους workers η αποθήκευση των εγγραφών γίνεται σε red black tree, το
οποίο έχει πολυπλοκότητα αναζήτησης και εισαγωγής, τόσο στην μέση, όσο και
στην χειρότερη περίπτωση O(logn).

-Οι ημερομηνίες που δέχεται στο πρόγραμμα στην είσοδο είναι της μορφής:
DD-MM-YYYY
Για να γίνει ο έλεγχος των διαστημάτων εξετάζεται αν η ημερομηνία
εισαγωγής είναι μέσα στο διάστημα που ζητήθηκε.

-Χρησιμοποιήθηκε handling για τα signals SIGINT και SIGCHLD στον master, ώστε να 
καθαρίζεται η μνήμη στην περίπτωση του πρώτου και να δημιουργείται νέος worker στην 
περίπτωση του δεύτερου.

-Ο worker αφού υπολογίσει τα αποτελέσματα, τα στέλνει στον server σε ένα 
μήνυμα (δηλαδή ένα write), το οποίο αποτελείται απο μικρότερα μηνύματα χωρισμένα 
με τον χαρακτήρα "$". Αν στην αρχή του μηνύματος υπάρχει το "-1", τότε ο  server
αντιλαμβάνεται ότι αυτό που ζήτησε δεν μπόρεσε να βρεθεί. Με τον ίδιο τρόπο επικοινωνεί
και ο ίδιος με τον client. 

-Ο client αρχικά διαβάζει μια φορά το αρχείο και μετρά τις ερωτήσεις. Στην συνέχεια τρέχει
ταυτόχρονα numThreads και οσο υπάρχουν ακόμα εντολές τρέχει ταυτόχρονα κάθε numThreads. Το 
κομμάτι του κώδικα που υλοποιεί το ταυτόχρονο ξεκίνημα των threads είναι εμπνευσμένο από 
διαδικτυακή πηγή (stackoverflow).  
