# Relazione progetto Big Farm.
### Studente: Daniele Bedini.
## Processo Master-Worker, exe: farm.
Questo è il modo in cui ho gestito MasterWorker:

### Farm:
Dato che il processo deve gestire il segnale SIGINT, ho dichiarato:

- (Prima di main): volatile sig_atomic_t sig_arrived = 0.
- (Prima di main): la funzione void signal_handler(int sig) che pone a 1 il valore della variabile.
- (appena dentro il main): la struct "sigaction" che se l'utente invia il segnale chiama la funzione signal_handler.

Gestione parametri opzionali (nel main):

- Uso della funzione di libreria getopt() per leggere e gestire i parametri opzionali.
- Variabili globali per l'uso di getopt(): "extern char *optarg" e "extern int optind".
- Per il flag "-n" e "-q" ho semplicemente utilizzato la funzione "atoi(optarg)" per convertire la stringa in numero, questo non da problemi nel caso in cui non sia valido il numero perché subito dopo ho aggiunto una "assert(qlen > 0)" e "assert(tn>0)", in questo caso accetta solo numeri maggiori strettamente a 0.
- Nel caso invece del flag "-t" ho utilizzato la funzione "strtol(optarg, &endptr,10)" che converte la stringa in numero in base 10. Se viene passato un valore non valido in questo caso con un controllo su "endptr" me ne accorgo, in questo caso il programma termina.

Gestione Worker threads:

- Definisco la struct "wdata" che servirà ai thread per avere i dati necessari per operare.
- Inizializzo: il buffer con la lunghezza di default o la lunghezza specificata nel flag opzionale "-q", gli indici di produttore e consumatori, i semafori necessari e i thread da lanciare con il numero di default oppure il numero del flag opzionale "-n".
- Lancio i thread assegnando ad ognuno i valori necessari specifcati dalla struct, con la funzione "xpthread_create(&wt[i],NULL,wtbody,&wdata[i],__ LINE __ , __ FILE __);"
- Scrivo i nomi dei file passati a linea di comando nel buffer condiviso con i thread utilizzando i semafori. Accedo al buffer con la funzione wait sul semaforo "sem_free_slots", scrivo il nome ed esco con la funzione e esco dal buffer con la funzione post sul semaforo "sem_data_items".
La definizione del for utilizza la variabile "optind" che mi permette di saltare i parametri opzionali e quindi di scrivere nel buffer soltanto i nomi dei file, inoltre vi èla funzione "usleep(del*1000)" per indicare il ritardo tra una scrittura e un altra in millisecondi.
- Dopo aver scritto i nomi dei file viene scritto "-1" per indicare ai thread che non devono più leggere nel buffer, l'accesso al buffer è gestito con i semafori come il passaggio preceedente.
- Infine viene fatta la join dei thread per farli terminare tutti e vengono distrutti i semafori utilizzati e il mutex.

### Worker Threads

- Ogni thread lanciato esegue la funzione "wtbody".
- Dopo aver fatto il cast per il passaggio della struct, vengono inizializzate le variabili per il nome del file e per la somma.
- Viene prelevato il nome del file sia con l'uso del semaforo (wait sul semaforo sem_data_items) sia con l'uso della "xpthread_mutex_lock" per fare in modo che vengano prelevati i nomi in mutua esclusione sul buffer da parte di ogni thread, si preleva il nome del file e si incrementa l'indice del consumatore; segue una unlock sul mutex e una post su sem_free_data_items.
- Subito dopo si verifica che non si sia letto "-1" con una "!strcmp(fn, "-1")" all'interno di un if, se si legge "-1" si interrompe il lavoro del thread.
- Appena letto il nome del file si esegue una fopen con "rb" come secondo argomento per leggere file binari.
- Se l'apertura va a buon fine si calcola la somma finale con un do-while finché la fread non finisce di leggere nel file. Se l'apertura non va a buon fine si visualizza su stderr un errore di percorso del file, subito dopo si chiude il file con una fclose.
- A questo punto si apre la connessione con il socket di comunicazione con il server. Si inizializza il socket file descriptor, l'indirizzo del server e la variabile "size_t e" per il controllo degli errori. Dopodichè si crea il socket, gli si assegna l'indirizzo e la porta e si apre la connessione con la funzione connect.
- Si crea la richiesta "MW" da inviare al server, prima si invia la lunghezza della richiesta, poi la richiesta stessa.
- Siccome bisogna inviare la somma a 4 byte alla volta nel socket e i long sono da 8 byte, occorre dividere il long in due metà. Dopo avere inizializzato le due metà e aver calcolato, anche tramite l'uso di uno shift a destra di 32 bit, si inviano le due metà al server.
- Dopo aver inviato le somme si invia la lunghezza del file e il nome del file stesso.
- Infine si chiama la pthread_exit.

### Client


Se viene chiamato senza argomenti:
- Apre la connessione con il server tramite un socket inviando soltanto la lunghezza della richiesta e la richiesta C1.
- Legge poi la lunghezza della lista che il server ha generato dopo che il MaserWorker ha passato i dati.
- Se la lunghezza della lista è 0 scrive su "stdout" la stringaa "Nessun file."
- Se la lunghezza non è 0, procede con la lettura dei dati nel socket che sono stati inviati dal server.
- La somma la legge come stringa, prima legge la lunghezza della somma, poi la somma stessa.
- Stessa cosa ovviamente per il nome del file corrispondente.
- Infine stampa le stringhe somma e nome file su "stdout".
- Questi passaggi sono fatti tante volte quanto è la lunghezza della lista del server.

Se è chiamato con argomenti a linea di comando:
- Con strtol si salva in un long la somma scritta passata.
- Si apre la connessione con il socket e si invia la richiesta C2.
- Si invia ogni somma richiesta al server, una metà alla volta.
- Qua inizia la lettura di ciò che è stato restituito dal server nel socket nello stesso modo in cui è stata fatta per la richiesta C1 sopra.

### Collector

- Si dichiara la classe ClientThread che ci permette di inizializzare i thread che andranno a gestire ogni richiesta ricevuta.
- Nel main si inizializza il socket, si fa il bind tra indirizzo e porta e rimane in ascolto. Appena riceve una connessione fa partire un thread.
- La classe del thread a sua volta fa partire la funzione "handle_conn" per gestire la richiesta
- Letta la richiesta dal socket viene controllata.
- Se la richiesta è "MW", viene letta la somma e il file che sono stati inviati nel socket e viene chiamata la funzione "mw_req", passandole anche la somma e il nome del file come parametro. Quest'ultima si occupa di aggiungere alla lista "c_list" la coppia summa e nome file.
- Se la richiesta è "C1" viene chiamata la funzione "req_c1". Quest'ultima scrive ogni coppia presente nella lista c_list nel socket. Somma e nome file sono scritte come stringhe, quindi prima viene inviata la lunghezza della stringa e poi la somma/nome file.
- Se la richiesta è "C2" viene chiamata la funzione "req_c2" con anche la somma letta come parametro. Questa funzione crea una lista nuova dove verranno aggiunte le coppie nella lista c_list che hanno quella data somma. Viene ordinata con "sort" in modo crescente e ogni coppia è inviata al client tramite il socket.   
