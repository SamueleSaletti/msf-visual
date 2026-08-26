//include guard per evitare errori di duplicazione nel caso venisse includo l'header nel main più volte 
#ifndef GRAFO_H
#define GRAFO_H

#define _GNU_SOURCE
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include "xerrori.h"


// funzione di terminazione in caso di errore 
void termina(const char *messaggio);


// struct che definisce un arco, il puntatore NEXT serve per le liste concatenate della hash table (gestione collisioni)
// msf è true solamente per gli archi della minimum spanning forest
// u<v per convenzione, weight può essere negativo
typedef struct arco{
    int u, v;
    int weight;
    bool msf;
    struct arco *next;
} arco;


typedef struct elemento{
  int id;   // indice del nodo       
  int w;    // peso 
  bool msf; // true sse questo arco appartiene alla MSF
  struct elemento *next;
} elemento;

//prototipi delle funzioni per calcolare la MSF, devo rappresentare il grafo come: 
// insieme di archi: tabella hash "gHash" contente puntatori a struct arco 
// liste di adiacenza (per nodi): array "vicini" che hanno una linked list di struct elemento per ogni nodo, ordine CRESCENTE dei nodi
// tabella delle componenti connesse (per nodi): array "cCon", per ogni nodo contiene l'ID della componente connessa a cui appartiene (indice nodo + piccolo)

typedef struct {
  arco **gHash;       // tabella hash (array di liste di archi)
  elemento **vicini;  // array di liste di adiacenza
  int *cCon;          // array delle componenti connesse
  int numCoCo;        // numero di componente connesse
  long costoMSF;      // costo della MSF 
  
  int V; //numero nodi
  int E; // numero archi
  int hashsize; // non viene modificato
  // CAMPI MULTITHREADING 
  //array di dimensione nmutex per proteggere l'accesso alle entry della hash table
  pthread_mutex_t *mut_gHash;
  int nmutex; // anche questo non viene modificato

  bool *componente_busy;
  pthread_mutex_t mutex_comp; // protegge numcoco, costomsf, ccon, componente_busy
  pthread_cond_t cv_comp;  
} grafo;

// struct usata dalla union find per gestire i set, se un elemento corrisponde al parent è la root di quell'insieme
typedef struct {
    int parent;
    int rank;         // usato per la union by rank
    int min_id;       // usato per popolare più velocemente l'array cCon, visto che la root potrebbe non essere l'elemento con id minim
} nodo_union;


//struct per il tipo di operazione da inserire nel buffer produttori/consumatori
typedef struct {
    char type;
    int u;
    int v;
    int w;
} operazione;

// struct per i parametri di input dei threads consumatori
typedef struct {
    grafo *grafo;       //passo un puntatore al grafo costruito che condividono i threads
    operazione *buffer;        // puntatore al buffer prod/cons, gli elementi sono le operazioni che devono svolgere i consumatori
    int *pcindex;       // putnatore al index del consumatore
    pthread_mutex_t *pmutex; // puntatore al mutex che protegge il buffer
    sem_t *sem_free_slots; //puntatore al semaforo di sync, inzializzato a Buf_size
    sem_t *sem_data_items; // inizalizzato a zero
    int buffer_size;
} dati;

typedef struct nodo_coda{
    int id;
    struct nodo_coda *next;
} nodo_coda;

typedef struct {
    nodo_coda *head;
    nodo_coda *tail;
} coda_bfs;

arco **parse_file(FILE *f, int *nodi, int *archi);

// ALGORITMO DI KRUSKAL, dato il grafo ritorna la msf usando una disjoint-set ds per trovare se due nodi fanno parte dello stesso albero (e individuare i cicli)
int confronta_archi(const void*a, const void*b);
int find(nodo_union *array_nodi, int id_nodo);
void union_rank(nodo_union *array_nodi, int root_x, int root_y,int*numCoCo);
arco **kruskal(arco**array_archi, int **cCon,int n_nodi,int n_archi, int*numCoCo);


//funzione che dato l'array di archi parsato mi ritorna la rappresentazione del grafo applicando kruskal 
//e popolando gHash, vicini e cCon, calcolando poi costoMSF e numCoCo
int hash_arco(int u, int v, int hashsize);
void inserisci_ordinato(elemento **testa,int v, int w, bool msf);
grafo crea_grafo(arco **array_archi, int n_nodi, int n_archi, int hashsize);

// calcola il numero primo che precede n
int primo_precedente(int n);
bool is_prime(int n);

//----------------------------------------------------------

//funzioni usate per cancella arco dei threads consumatori
bool is_empty(coda_bfs *q);
void enqueue(coda_bfs *q, int id_nodo);
int dequeue(coda_bfs *q);

void aggiorna_gHash(arco **gHash,int h,int best_u, int best_v);
bool rimuovi_da_ghash(arco **gHash, int h, int u, int v, bool *era_msf, int *peso_rimosso);
void aggiorna_vicini(elemento **vicini, int id1, int id2);
void rimuovi_da_vicini(elemento **vicini, int id1, int id2);
bool *bfs_msf(grafo *g, int id_start);

bool cancella_arco(grafo *graph, int u, int v);

void *consumer_op(void *arg);

//------ funzioni di ricalcolo ------

typedef struct{
    //calcolate con visita a gHash
    int n_archi;
    long costo_msf;
    int pos_piene;
    float l_media;
    int l_max;

    //calcolata con visita a cCon
    int n_comp;
} ricalcolo;

int confronta_int(const void *a, const void *b);
ricalcolo calcolo_finale(grafo *g);

#endif 