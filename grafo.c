#define _GNU_SOURCE 
#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include "grafo.h"
#include <assert.h>

#define QUI __LINE__,__FILE__



void termina(const char *messaggio)
{
  if(errno==0) 
     fprintf(stderr,"%s\n",messaggio);
  else 
    perror(messaggio);
  exit(1);
}

bool is_prime(int n){
    if(n<2) return false;
    for(int i=2;i*i <= n; i++){
        if(n%i == 0) return false;
    }
    return true;
}

int primo_precedente(int n){
    int primo;
    if (n==3) return 2;
    if(n%2) primo=n;
    else primo = n-1;
    while(!is_prime(primo)){
        primo -= 2;
    }
    return primo;
}

arco **parse_file(FILE *f, int *nodi, int *archi){
    //usate da getline()
    char *buffer = NULL;
    size_t n=0;

    arco **array_archi = NULL;
    // per scorrere l'array di struct
    int i_corrente = 0;

    while(true){
        ssize_t e = getline(&buffer,&n,f);
        //ritorna il numero di caratteri letti, ritorna -1 in caso di errore o EOF
        if(e<0){
            if(buffer!=NULL) free(buffer);
            break;
        }
        if(e<=1 || buffer[0] == 'c') continue;

        char *s = strtok(buffer," \n");
        //se leggo linea vuota
        if(s==NULL) continue;
        // se la linea inizia con p
        if(strcmp(s,"p")==0){
            // skippo sp e vado avanti
            s = strtok(NULL," \n");
            *nodi = atoi(strtok(NULL," \n")) + 1;
            *archi = atoi(strtok(NULL," \n"));
            
            // array che poi restituisco alla fine di questa funzione 
            array_archi = calloc(*archi, sizeof(arco*)); 
            if(array_archi == NULL) termina("malloc array di archi fallita durante il parsing");

        } else if (strcmp(s,"a")== 0){
            if(array_archi!=NULL && i_corrente<(*archi)){
                arco *arco_temp = malloc(sizeof(arco));
                if(arco_temp == NULL) termina("malloc singolo arco fallita durante il parsing");
                arco_temp->u = atoi(strtok(NULL," \n"));
                arco_temp->v = atoi(strtok(NULL," \n"));
                arco_temp->weight = atoi(strtok(NULL," \n"));
                arco_temp->msf = false;  //inizializzo tutti gli archi a false

                array_archi[i_corrente] = arco_temp;
                i_corrente++;

            }
            
        }
    }

    return array_archi;
}

// ------------------------ INIZIO COSTRUZIONE GRAFO ----------------------

int confronta_archi(const void*a, const void*b){
    // a e b sono puntatori a elementi dell'array che sono a loro volta puntatori quindi saranno di tipo arco**
    const arco *arco_a= *(arco**) a; //devo deferenziare il puntatore per ottenere il putnatore all'arco
    const arco *arco_b = *(arco**) b;
    if(arco_a->weight < arco_b->weight) return -1;
    if(arco_a->weight > arco_b->weight) return 1;
    
    // Criterio di spareggio deterministico sugli ID
    if(arco_a->u < arco_b->u) return -1;
    if(arco_a->u > arco_b->u) return 1;
    if(arco_a->v < arco_b->v) return -1;
    if(arco_a->v > arco_b->v) return 1;
    return 0;
}

// ritorna l'id del nodo root del set a cui appartiene id_nodo 
int find(nodo_union *array_nodi, int id_nodo){
    int root = id_nodo;
    //ciclo che trova la root del sottoinsieme a cui appartiene id_nodo
    while(array_nodi[root].parent != root){
        root = array_nodi[root].parent;
    }

    //ciclo che aggiorna con root il parent di tutti i nodi sul cammino tra id_noido e root
    //PATH COMPRESSION
    while(array_nodi[id_nodo].parent != root){
        int parent = array_nodi[id_nodo].parent;
        array_nodi[id_nodo].parent = root;
        id_nodo = parent;
    }

    return root;
}

//unisco le due comoponenti se le root dei due nodi che sto trattando sono diverse
void union_rank(nodo_union *array_nodi, int root_x, int root_y, int*numCoCo){

    if(root_x == root_y) return;

    // aggiorno il nuovo minimo nella radice, mi basterà per ogni nodo trovare la sua radice e saprò l'id minino di quella componente
    int nuovo_minimo = (array_nodi[root_x].min_id < array_nodi[root_y].min_id) ? array_nodi[root_x].min_id : array_nodi[root_y].min_id;

    

    // quella con rank più alto diventa il padre
    if(array_nodi[root_x].rank < array_nodi[root_y].rank){
        array_nodi[root_x].parent = root_y;
        array_nodi[root_y].min_id = nuovo_minimo;
    } else if (array_nodi[root_x].rank > array_nodi[root_y].rank){
        array_nodi[root_y].parent = root_x;
        array_nodi[root_x].min_id = nuovo_minimo;
    } else {
        array_nodi[root_y].parent = root_x;
        array_nodi[root_x].rank += 1;
        array_nodi[root_x].min_id = nuovo_minimo;
    }
    // se faccio merge decremenento le componenti connesse
    *numCoCo -= 1;

}
// passo anche puntatore a numCoCo per sovrascriverlo durante la Union
arco **kruskal(arco**array_archi, int **cCon,int n_nodi,int n_archi,int *numCoCo){

    //ora array_archi è ordinato per ordine di peso crescente
    qsort(array_archi,n_archi,sizeof(arco*),&confronta_archi);

    //alloco un array di nodo_union di grandezza n_nodi per operare con la union_find
    nodo_union *array_nodi = malloc(n_nodi*sizeof(nodo_union));
    if(array_nodi == NULL) termina("errore malloc array nodi in kruskal");

    // faccio MakeSet per ogni nodo 
    for(int i=0;i<n_nodi;i++){
        array_nodi[i].parent = i;
        array_nodi[i].rank = 0;
        array_nodi[i].min_id = i;
    }

    // per ogni arco se il risultato della find sui due nodi è diverso allora setto il campo msf a true e fa la union 
    for(int i=0;i<n_archi;i++){
        int root_u = find(array_nodi,array_archi[i]->u);
        int root_v = find(array_nodi,array_archi[i]->v);
        if(root_u != root_v){
            //aggiungo l'arco alla msf
            array_archi[i]->msf = true;
            union_rank(array_nodi, root_u, root_v, numCoCo);

        }
    }

    //popolo cCon
    for(int i=0; i<n_nodi;i++){
        //minimo è il valore minimo della componente connessa a cui appartiene il nodo i
        int minimo = array_nodi[find(array_nodi,i)].min_id;
        (*cCon)[i] = minimo;
    }

    //dealloco array nodi

    free(array_nodi);
    //posso ritornare l'array di archi con tutti i campi msf settati, questo array sarà quello temporaneo per poplare le altre strutture
    return array_archi;
}

int hash_arco(int u, int v, int hashsize){
    //calcolo la chiave in questo modo
    int min_node = (u < v) ? u : v;
    int max_node = (u > v) ? u : v;
    long h = min_node * 31 + max_node;
    return h % hashsize;
}

//dato il puntatore alla testa della lista del nodo corrente, crea l'elemento e lo inserisce nella lista
void inserisci_ordinato(elemento **testa,int v, int w, bool msf){
    elemento *nuovo = malloc(sizeof(elemento));
    nuovo->id = v;
    nuovo->w = w;
    nuovo->msf = msf;
    nuovo->next = NULL;

    // *testa è il puntatore al primo nodo, sarebbe vicini[u]
    if(*testa == NULL || (*testa)->id >= v){
        nuovo->next = *testa;
        *testa = nuovo;
        return;
    }

    // scorro la lista fino a quando l'elemento dopo quello corrente sarà NULL o avrà indice > di quello di v
    elemento *corrente= *testa;
    while(corrente->next!=NULL && corrente->next->id < v){
        corrente = corrente->next;
    }

    nuovo->next = corrente->next;
    corrente->next = nuovo;
}

// n_nodi serve a qsort e disjoint-set, n_archi serve a kruskal per iterare
grafo crea_grafo(arco **array_archi, int n_nodi, int n_archi, int hashsize){
    grafo graph;
    
    int *cCon = malloc(n_nodi*sizeof(int));
    int numCoCo = n_nodi; //all'inizio il numero di componenti connesse è uguale al numero di nodi, ogni volta che faccio merge di componenti decremento questo valore
    if(cCon==NULL) termina("errore malloc array cCon durante creazione grafo");


    //ritorna la lista di archi con tutti i valori msf settati correttamente, popola cCon
    arco **array_archi_msf = kruskal(array_archi,&cCon,n_nodi,n_archi,&numCoCo);

    //crea tabella hash e calcola il costo della msf 
    // array di hashsize elementi che puntano tutti a NULL all'inizio
    long costomsf = 0;
    arco **gHash = calloc(hashsize,sizeof(arco *));
    if(gHash == NULL) termina("errore calloc allocazione hash table durante creazione grafo");

    for(int i=0; i<n_archi;i++){
        arco *arco_corrente = array_archi_msf[i];

        if(arco_corrente->msf == true){
            costomsf = costomsf + arco_corrente->weight;
        }
        
        //inserimento nella tabella hash
        int pos_hash = hash_arco(arco_corrente->u,arco_corrente->v,hashsize);
        arco_corrente->next = gHash[pos_hash];
        gHash[pos_hash] = arco_corrente;
    }

    //crea lista di adiacenza 
    elemento **vicini = calloc(n_nodi,sizeof(elemento *));
    for(int i=0; i<n_archi;i++){
        int u = array_archi_msf[i]->u;
        int v = array_archi_msf[i]->v;
        int w = array_archi_msf[i]->weight;
        bool msf = array_archi_msf[i]->msf;

        inserisci_ordinato(&vicini[u],v,w,msf);
        inserisci_ordinato(&vicini[v],u,w,msf);
    }

    graph.gHash = gHash;
    graph.cCon = cCon;
    graph.vicini = vicini;
    graph.numCoCo = numCoCo;
    graph.costoMSF = costomsf;
    graph.V = n_nodi;
    graph.E = n_archi;
    graph.hashsize = hashsize;

    //libero l'array di puntatori e ma non gli archi che sono dentro la hash table
    free(array_archi_msf);
    return graph;
}


//------------------ metodi della coda bfs -----------------------------

// se la head è null la coda è vuota
bool is_empty(coda_bfs *q){
    return q->head == NULL;
}

// inserisce in coda 
void enqueue(coda_bfs *q, int id_nodo){
    nodo_coda *nuovo_nodo = malloc(sizeof(nodo_coda));
    nuovo_nodo->id = id_nodo;
    nuovo_nodo->next = NULL;

    if(is_empty(q)){
        q->head = nuovo_nodo;
        q->tail = nuovo_nodo;
    } else {
        q->tail->next = nuovo_nodo;
        q->tail = nuovo_nodo;
    }
}

// elimina un nodo dalla testa e ritorna il suo id
int dequeue(coda_bfs *q){
    nodo_coda *temp = q->head;
    int id_ret = temp->id;
    q->head = temp->next;

    if(q->head == NULL){
        q->tail = NULL;
    }
    free(temp);
    return id_ret;
}



// ================= FUNZIONI THREAD CONSUMATORI ========================

void aggiorna_gHash(arco **gHash,int h,int best_u, int best_v){
    arco *curr = gHash[h];
    while(curr != NULL){
        if((curr->u == best_u && curr->v == best_v) || (curr->u == best_v && curr->v == best_u)){
            assert(curr->msf == false);
            curr->msf = true;
            break;
        }
        curr = curr->next;
    }
}


// ritorna true se l'ho rimosso correttamente e mi dice se era nella msf o no
bool rimuovi_da_ghash(arco **gHash, int h, int u, int v, bool *era_msf, int *peso_rimosso){
    arco *curr = gHash[h];
    arco *prec = NULL;
    while(curr != NULL){
        if((curr->u == u && curr->v == v) || (curr->u == v && curr->v == u)){
            //se sto eliminando la testa
            if(prec == NULL){
                gHash[h] = curr->next;
                *era_msf = curr->msf;
                *peso_rimosso = curr->weight;
                free(curr);
                return true;
            } else {
                // sono in mezzo alla lista
                prec->next = curr->next;
                *era_msf = curr->msf;
                *peso_rimosso = curr->weight;
                free(curr);
                return true;
            }
        }
        prec = curr;
        curr = curr->next;
    }

    //arco non trovato, esito negativo
    return false;
}

//aggiorna vicini mettendo il campo msf=true per l'arco da id1 a id2
void aggiorna_vicini(elemento **vicini, int id1, int id2){
    elemento *curr = vicini[id1];
    while(curr!=NULL){
        if(curr->id == id2){
            curr->msf = true;
            break;
        }
        curr = curr->next;
    }
}

// rimuove elemento con id2 da vicini[id1]
void rimuovi_da_vicini(elemento **vicini, int id1, int id2){
    elemento *curr = vicini[id1];
    elemento *prec = NULL;
    while(curr != NULL){
        if(curr->id == id2){
            if(prec == NULL){
                vicini[id1] = curr->next;
            } else {
                prec->next = curr->next;
            }
            // quando cancello il nodo ho finito
            free(curr);
            return;
        }
        prec = curr;
        curr = curr->next;
    }
}

// ritorna la lista dei nodi visitabili a partire dal nodo id_start
bool *bfs_msf(grafo *g, int id_start){
    // alla fine coinciderà con Lu o Lv a seconda del nodo di partenza
    bool *visitati = calloc(g->V, sizeof(bool));
    if(visitati == NULL) termina("errore malloc array nodi visitati dentro la visita bsf");

    //inizializzo la coda vuota
    coda_bfs coda;
    coda.head = NULL;
    coda.tail = NULL;

    enqueue(&coda, id_start);
    visitati[id_start] = true;

    //inizio ciclo bfs
    while(!is_empty(&coda)){
        int curr = dequeue(&coda);
        //esploro i vicini del nodo e li metto visitati se il campo msf è true e non sono già stati visitati
        elemento *vic = g->vicini[curr];
        while(vic!=NULL){
            if(vic->msf == true && visitati[vic->id] == false){
                visitati[vic->id] = true;
                enqueue(&coda, vic->id);
            }
            
            // continuo la visita dei vicini
            vic = vic->next;
        }
    }
    return visitati;
}

// funzione che cancella l'arco dal grafo aggiorando tutte le componenti
// ritorna l'ESITO: true se l'operazione era valida, false se cancello arco non esistente 
bool cancella_arco(grafo *graph, int u, int v){
    //mutex che protegge tutti i parametri del grafo
    xpthread_mutex_lock(&graph->mutex_comp,QUI);
    
    if(u<0 || u >= graph->V || v<0 || v >= graph->V){
        xpthread_mutex_unlock(&graph->mutex_comp,QUI);
        return false; //OPERAZIONE NON VALIDA
    } 

    // id della componente connessa a cui appartengono u e v
    int c1 = graph->cCon[u];
    int c2 = graph->cCon[v];

    //se una delle due componenti è busy mi metto in attesa sulla cv
    while(graph->componente_busy[c1] || graph->componente_busy[c2]){
        xpthread_cond_wait(&graph->cv_comp, &graph->mutex_comp, QUI);
        // ricalcolo le componenti al risveglio
        c1 = graph->cCon[u];
        c2 = graph->cCon[v];
    }

    //metto busy le due componenti e rilascio la lock così altri thread possono operare 
    //mentre faccio gli altri calcoli
    graph->componente_busy[c1] = true;
    if(c1!=c2) graph->componente_busy[c2] = true;
    xpthread_mutex_unlock(&graph->mutex_comp,QUI);

    // ADESSO HO IL CONTROLLO DI TUTTA LA COMPONENTE, quindi di vicini della componente e anche cCon

    bool era_msf = false;
    int peso_rimosso = 0;
    // cancello l'arco dalla tabella hash
    int h = hash_arco(u,v,graph->hashsize);
    xpthread_mutex_lock(&graph->mut_gHash[h % graph->nmutex],QUI);
    bool esito = rimuovi_da_ghash(graph->gHash, h, u, v, &era_msf, &peso_rimosso);
    xpthread_mutex_unlock(&graph->mut_gHash[h % graph->nmutex],QUI);

    if(esito) {
        rimuovi_da_vicini(graph->vicini, u ,v);
        rimuovi_da_vicini(graph->vicini, v, u);
    }

    //se non era msf, libero le componenti e ritorno l'esito
    if(!era_msf){
        xpthread_mutex_lock(&graph->mutex_comp, QUI);
        graph->componente_busy[c1] = false;
        if(c1 != c2) graph->componente_busy[c2] = false;

        if (esito == true) {
            graph->E --; 
        }
        xpthread_cond_broadcast(&graph->cv_comp, QUI);

        //stampo op | u | v | E | numCoCo | costoMSF
        printf("- %d %d %d %d %ld \n", u, v, graph->E, graph->numCoCo, graph->costoMSF);
        xpthread_mutex_unlock(&graph->mutex_comp,QUI);
        return esito;
    } 

    // l'arco era nella msf, faccio una visita della msf a partire da u e poi da v
    bool *Lu = bfs_msf(graph, u);   //DA DEALLOCARE
    bool *Lv = bfs_msf(graph, v);   // DA DEALLOCARE

    //devo cercare l'arco di costo minimo che collega un nodo di Lu con uno di Lv
    // scorro Lu, se il nodo è true scorro la sua lista in vicini e se trovo un id che è true in Lv allora è un arco valido

    int best_u = -1; //indice dei nodi collegati dall'arco di costo minimo che collega le due componenti
    int best_v = -1;
    int min_peso = __INT_MAX__;

    for(int i=0; i<graph->V; i++){
        if(Lu[i]==true){
            elemento *vic = graph->vicini[i];
            while(vic != NULL){
                if(Lv[vic->id] == true){
                    if(vic->w < min_peso){
                        min_peso = vic->w;
                        best_u = i;
                        best_v = vic->id;
                    }
                }

                //vado avanti a scorrere la lista
                vic = vic->next;
            }
        }
    }

    int differenza_costo_msf = -peso_rimosso; // devo sommarci min_peso se l'ho aggiunto, altrimenti niente
    bool ho_splittato = false;
    //sono i primi id in Lu e Lv con campo true
    int nuovo_id_u = -1;
    int nuovo_id_v = -1;

    if(best_u != -1){
        //in questo caso ho trovato l'arco di costo minimo, devo aggiungerlo alla msf in ghash e vicini
        int h = hash_arco(best_u,best_v,graph->hashsize);
        xpthread_mutex_lock(&graph->mut_gHash[h % graph->nmutex],QUI);
        aggiorna_gHash(graph->gHash, h, best_u, best_v);
        xpthread_mutex_unlock(&graph->mut_gHash[h % graph->nmutex],QUI);

        aggiorna_vicini(graph->vicini, best_u, best_v);
        aggiorna_vicini(graph->vicini, best_v, best_u);

        differenza_costo_msf += min_peso; //siccome l'ho aggiunto alla msf 
    } else{
        //in questo caso la componente si è spezzata, devo ricalcolare cCon
        
        

        for(int i=0; i<graph->V; i++){
            if(Lu[i] == true){
                nuovo_id_u = i;
                break;
            }
        }
        for(int i=0; i<graph->V; i++){
            if(Lv[i] == true){
                nuovo_id_v = i;
                break;
            }
        }

        ho_splittato = true;

    }

    xpthread_mutex_lock(&graph->mutex_comp,QUI);
    graph->E--;
    graph->costoMSF += differenza_costo_msf;
    if(ho_splittato) {
        graph->numCoCo ++;
        for(int i=0; i<graph->V; i++){
            if(Lu[i]== true){
                graph->cCon[i] = nuovo_id_u;
            } else if(Lv[i] == true){
                graph->cCon[i] = nuovo_id_v;
            }
        }
    }
    graph->componente_busy[c1] = false;
    if(c1!=c2) graph->componente_busy[c2] = false;

    xpthread_cond_broadcast(&graph->cv_comp,QUI);

    //stampo op | u | v | E | numCoCo | costoMSF
    printf("- %d %d %d %d %ld \n", u, v, graph->E, graph->numCoCo, graph->costoMSF);
    xpthread_mutex_unlock(&graph->mutex_comp,QUI);
    
    free(Lu);
    free(Lv);
    return esito;

}


void *consumer_op(void *arg){
    dati *a = (dati *)arg;
    
    operazione op;
    do{
        //estraggo l'operazione dal buffer
        xsem_wait(a->sem_data_items,QUI);
        xpthread_mutex_lock(a->pmutex, QUI);
        op = a->buffer[*(a->pcindex) % a->buffer_size];
        *(a->pcindex) +=1;
        xpthread_mutex_unlock(a->pmutex,QUI);
        xsem_post(a->sem_free_slots,QUI);

        bool esito;
        if(op.type == '+'){
            printf("%c %d %d %d 0 \n", op.type, op.u, op.v, op.w);
            continue;
        } else if (op.type == '-'){
            esito = cancella_arco(a->grafo,op.u,op.v);
            if(!esito){
                printf("%c %d %d 0 \n", op.type, op.u, op.v);
            }
        }

        
    } while(op.type != 'T');
    
    // termino il thread
    return NULL;
}


// ======================== OPERAZIONI DI RICALCOLO ===========================

int confronta_int(const void *a, const void *b){
    int arg1 = *(const int *)a;
    int arg2 = *(const int *)b;
    if(arg1<arg2) return -1;
    if(arg1>arg2) return 1;
    return 0;
}

ricalcolo calcolo_finale(grafo *g){
    ricalcolo ric;
    int n_archi = 0;
    long costo_msf = 0;
    int pos_piene = 0;
    int l_max = 0;
    float l_media;

    //l_media sarà (n_archi / pos_piene)
    // gHash ha dimensione hashsize
    
    for(int i=0;i<g->hashsize; i++){
        //lista i della tabella hash
        arco *curr = g->gHash[i];
        if(curr==NULL){
            continue;
        }
        int l_temp = 0;
        while(curr!=NULL){
            n_archi ++;
            if(curr->msf) costo_msf += curr->weight;
            l_temp ++;
            curr = curr->next;
        }
        pos_piene++;
        if(l_temp>l_max) l_max = l_temp;
    }

    if(pos_piene > 0){
        l_media = (float)n_archi / pos_piene;
    } else l_media = 0.0;

    int n_comp = 0;
    //calcolo del numero di componenti connesse con l'array cCon
    int *copy = malloc(g->V*sizeof(int));
    if(copy == NULL) termina("errore malloc array copia di cCon per ricalcolo componenti connesse");

    mempcpy(copy,g->cCon,g->V * sizeof(int));
    //ordino l'array copia e dopo calcolo il numero di elementi diversi
    qsort(copy, g->V, sizeof(int), confronta_int);
    
    if(g->V > 0) n_comp++;
    for(int i=1; i < g->V; i++){
        if(copy[i] != copy[i-1]){
            n_comp ++;
        }
    }

    free(copy);

    ric.n_archi = n_archi;
    ric.costo_msf = costo_msf;
    ric.pos_piene = pos_piene;
    ric.l_media = l_media;
    ric.l_max = l_max;
    ric.n_comp = n_comp;

    return ric;

}