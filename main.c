#include "grafo.h"
#include "xerrori.h"
#include <getopt.h>

#define DEFAULT_THREADS 3
#define DEFAULT_HASHSIZE 100000
#define DEFAULT_MUTEX 1000

#define Buf_size 100
#define QUI __LINE__,__FILE__

//l'accesso alla tabella gHash deve essere protetto da mutex, uso un array "mut_gHash" di mutex di nmutex elementi perchè non posso usare una mutex
// per ogni entry della tabella hash (di base sono 100k entry)

// CV+mutex+array di flags per far accedere un thread alla volta ad una componente connessa 

//  1 PRODUTTORE / t CONSUMATORI:  thread produttore legge op. di cancellazione, mette nel buffer, 
//  consumatori la prelevano e la eseguono concorrentemente

int main(int argc, char *argv[]){
    //uso getopt che ritorna -1 quando ha finito di trovare i parametri, ritorna ? se c'è un errore
    int opt;
    int num_threads = DEFAULT_THREADS;
    int hash_size = DEFAULT_HASHSIZE;
    int n_mutex = DEFAULT_MUTEX;

    //optarg contiene il valore passato, getopt riorganizza l'array argv
    while((opt = getopt(argc,argv,"t:H:M:")) != -1){
        switch(opt){
            case 't':
                num_threads = atoi(optarg);
                if(num_threads<=0){
                    termina("inserisci un numero di thread >0");
                }
                break;

            case 'H':
                hash_size = atoi(optarg);
                if(hash_size<2) termina("inserisci hashsize >=2");
                break;

            case 'M':
                n_mutex = atoi(optarg);
                if(n_mutex<=0) termina("inserisci numero di mutex >0");
                break;

            case '?':
                termina("errore getopt");
                break;
        }
    }

    //optind è l'indice di argv del prossimo elemento da esaminare
    // se passo prima i file e poi i flag, getopt riorganizza argv mettendo prima i flag e poi i file 
    // alla fine optind è l'indice del primo elemento non flag di argv e argc-optind è il numero di elementi non flag
    if((argc - optind)!= 2){
        fprintf(stderr,"Uso %s file_grafo file_archi [-t threads] [-H hashsize] [-M nmutex] \n",argv[0]);
        exit(1);
    }

    hash_size = primo_precedente(hash_size);

    FILE *file_grafo = fopen(argv[optind],"r");
    if(file_grafo == NULL) termina("errore apertura file .gr");

    //funzione che dato il file mi ritorna un array di puntatori a struct arco(ognuna allocata con malloc)
    //questo è l'array da ordinare per usare kruskal, gli archi già allocati verranno messi nella tabella hash direttamente
    //con questo array creo ancje l'array VICINI, mentre per cCon uso la union_find
    int n_nodi = 0;
    int n_archi = 0;
    arco **archi_temporanei = parse_file(file_grafo, &n_nodi, &n_archi);
    if(fclose(file_grafo)!=0) termina("errore chiusura file grafo dopo il parsing");

    //n_nodi contiene già il numero del file +1 perchè è compreso anche il nodo con ID=0
    grafo graph = crea_grafo(archi_temporanei, n_nodi, n_archi, hash_size);

    // 1. STAMPA STATISTICHE INIZIALI
    fprintf(stdout, "{\"type\": \"init_stats\", \"n_archi\": %d, \"numCoCo\": %d, \"costoMSF\": %ld}\n", n_archi, graph.numCoCo, graph.costoMSF);
    fflush(stdout);

    // 2. STAMPA TUTTI GLI ARCHI USANDO LA NUOVA FUNZIONE
    stampa_grafo_json(&graph);

    // 3. ASPETTA IL VIA DAL WEB
    char cmd[20];
    if (fgets(cmd, sizeof(cmd), stdin) != NULL) {
        // Avviamo i threads
    }

    // ---------------------------------- ININZIO THREADS
    assert(num_threads>0);
    int cindex = 0;
    int pindex = 0;
    operazione buffer[Buf_size];
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_t threads[num_threads]; //inizializzo num_thread threads consumatori
    dati a[num_threads]; 
    sem_t sem_free_slots, sem_data_items;
    xsem_init(&sem_free_slots, 0, Buf_size,QUI);
    xsem_init(&sem_data_items, 0, 0, QUI);

    graph.nmutex = n_mutex;
    graph.mut_gHash = malloc(n_mutex*sizeof(pthread_mutex_t));
    if(graph.mut_gHash == NULL) termina("errore malloc array di mutex per le entry della tabella hash");
    for(int i=0; i<n_mutex;i++){
        //inizializzo le mutex per la hash table
        xpthread_mutex_init(&graph.mut_gHash[i],NULL,QUI);
    }

    // inizializzo tutte a false
    graph.componente_busy = calloc(n_nodi,sizeof(bool));
    if(graph.componente_busy == NULL) termina("errore calloc array di bool per le componenti busy del grafo");

    // inizializzo la mutex e la cv per la sincronizzazione, 
    //ora il grafo è pronto per essere passato ai consumatori che eseguiranno cancella_arco
    xpthread_mutex_init(&graph.mutex_comp,NULL,QUI);
    xpthread_cond_init(&graph.cv_comp, NULL, QUI);


    //faccio pardire num_threads consumatori
    for(int i=0; i<num_threads; i++){
        a[i].grafo = &graph;
        a[i].buffer = buffer;
        a[i].pcindex = &cindex;
        a[i].pmutex = &mutex;
        a[i].sem_free_slots = &sem_free_slots;
        a[i].sem_data_items = &sem_data_items;
        a[i].buffer_size = Buf_size;
        // il parametro attribute si usa per specificare thread detached
        xpthread_create(&threads[i], NULL, consumer_op, &a[i],QUI); //funzione consumatori da implementare
    }
    
    FILE *file_operazioni = fopen(argv[optind+1],"r");
    if(file_operazioni == NULL) termina("errore apertura file con operazioni");

    // devo leggere ogni linea di questo file e creare la struct operazione da mettere nel buffer
    char *buffer_getline = NULL;
    size_t n = 0;

    while(true){
        ssize_t e = getline(&buffer_getline, &n,file_operazioni);   //n è la dimensione del buffer allocato per la stringa
        if(e<0){
            if(buffer_getline!=NULL) free(buffer_getline);
            break; //EOF
        }
        char *s = strtok(buffer_getline," \n");
        if(s==NULL) continue;

        //caso add 
        if(strcmp(s,"+")==0){
            char *token_u = strtok(NULL, " \n");
            char *token_v = strtok(NULL, " \n");
            char *token_w = strtok(NULL, " \n");
            if(token_u==NULL || token_v==NULL || token_w == NULL ) continue;
            if (strtok(NULL, " \n") != NULL) continue;
            operazione temp;
            temp.type = '+';
            temp.u = atoi(token_u);
            temp.v = atoi(token_v);
            temp.w = atoi(token_w);
            xsem_wait(&sem_free_slots, QUI);
            buffer[pindex++ % Buf_size] = temp;
            xsem_post(&sem_data_items, QUI);
        } 
        //caso canc
        else if(strcmp(s,"-")==0){
            char *token_u = strtok(NULL, " \n");
            char *token_v = strtok(NULL, " \n");
            if(token_u==NULL || token_v==NULL) continue;
            if (strtok(NULL, " \n") != NULL) continue;
            operazione temp;
            temp.type = '-';
            temp.u = atoi(token_u);
            temp.v = atoi(token_v);
            xsem_wait(&sem_free_slots, QUI);
            buffer[pindex++ % Buf_size] = temp;
            xsem_post(&sem_data_items, QUI);
        }
    }
    if(fclose(file_operazioni)!=0) termina("errore chiusura file operazioni dopo il parsing");

    // adesso per ogni thread inserisco un terminatore (operazione con campo op == 'T')
    for(int i=0; i<num_threads;i++){
        operazione temp;
        temp.type = 'T';

        xsem_wait(&sem_free_slots, QUI);
        buffer[pindex++ % Buf_size] = temp;
        xsem_post(&sem_data_items, QUI);
    }

    // chiamo join su tutti i threads
    for(int i=0; i<num_threads; i++){
        xpthread_join(threads[i],NULL,QUI);
    }

    fprintf(stdout, "Operazioni terminate\n");

    // 4. STAMPE FINALI IN JSON
    ricalcolo ric = calcolo_finale(&graph);

    fprintf(stdout, "{\"type\": \"final_stats\", \"pos_piene\": %d, \"l_media\": %f, \"l_max\": %d, \"n_archi\": %d, \"n_comp\": %d, \"costo_msf\": %ld}\n",
            ric.pos_piene, ric.l_media, ric.l_max, ric.n_archi, ric.n_comp, ric.costo_msf);
    fflush(stdout);


    //------------------deallocazione variabili multithreading-----------------
    // semafori e mutex prod/cons
    xsem_destroy(&sem_free_slots,QUI);
    xsem_destroy(&sem_data_items,QUI);
    xpthread_mutex_destroy(&mutex,QUI);

    //parametri grafo
    for(int i=0; i < n_mutex; i++){
        xpthread_mutex_destroy(&graph.mut_gHash[i],QUI);
    }
    xpthread_mutex_destroy(&graph.mutex_comp,QUI);
    xpthread_cond_destroy(&graph.cv_comp, QUI);
    free(graph.componente_busy);
    free(graph.mut_gHash);


    //deallocazione array vicini scorrendo le liste, vedi lista_capitali.c (lsita_capitale_distruggi)
    for (int i = 0; i < n_nodi; i++) {
        elemento *current = graph.vicini[i];
        while (current != NULL) {
            elemento *temp = current->next;
            free(current);
            current = temp;
        }
    }
    
    // deallocazione liste interne tabella hash
    for(int i=0; i<hash_size;i++){
        arco *current = graph.gHash[i];
        while(current!=NULL){
            arco *temp = current->next;
            free(current);
            current = temp;
        }
    }

    free(graph.vicini);
    free(graph.gHash);
    free(graph.cCon);

    return 0;
}