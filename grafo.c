#define _GNU_SOURCE 
#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include "grafo.h"



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
            array_archi = malloc((*archi)* sizeof(arco*)); 
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

    //questo perchè la getline lo alloca dinamicamente
    if(buffer!=NULL) free(buffer);
    return array_archi;
}

// ------------------------ INIZIO COSTRUZIONE GRAFO ----------------------

int confronta_archi(const void*a, const void*b){
    // a e b sono puntatori a elementi dell'array che sono a loro volta puntatori quindi saranno di tipo arco**
    const arco *arco_a= *(arco**) a; //devo deferenziare il puntatore per ottenere il putnatore all'arco
    const arco *arco_b = *(arco**) b;
    if((arco_a->weight)<(arco_b->weight)) return -1;
    else if((arco_a->weight)>(arco_b->weight)) return 1;
    else return 0;
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
void union_rank(nodo_union *array_nodi, int root_x, int root_y){

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

}

arco **kruskal(arco**array_archi, int **cCon,int n_nodi,int n_archi){

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
            union_rank(array_nodi, root_u, root_v);

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

// n_nodi serve a qsort e disjoint-set, n_archi serve a kruskal per iterare
grafo crea_grafo(arco **array_archi, int n_nodi,int n_archi){
    int *cCon = malloc(n_nodi*sizeof(int));
    if(cCon==NULL) termina("errore malloc array cCon durante creazione grafo");


    //ritorna la lista di archi con tutti i valori msf settati correttamente, popola cCon
    arco **array_archi_msf = kruskal(array_archi,&cCon,n_nodi,n_archi);

    //crea tabella hash e calcola il costo della msf 

    //crea lista di adiacenza 



}
