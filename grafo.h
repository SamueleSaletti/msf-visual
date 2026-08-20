//include guard per evitare errori di duplicazione nel caso venisse includo l'header nel main più volte 
#ifndef GRAFO_H
#define GRAFO.H

#include <stdbool.h>
#include <string.h>
#include <stdio.h>


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
  // CAMPI MULTITHREADING 
} grafo;

// struct usata dalla union find per gestire i set, se un elemento corrisponde al parent è la root di quell'insieme
typedef struct {
    int parent;
    int rank;         // usato per la union by rank
    int min_id;       // usato per popolare più velocemente l'array cCon, visto che la root potrebbe non essere l'elemento con id minim
} nodo_union;

arco **parse_file(FILE *f, int *nodi, int *archi);

// ALGORITMO DI KRUSKAL, dato il grafo ritorna la msf usando una disjoint-set ds per trovare se due nodi fanno parte dello stesso albero (e individuare i cicli)
int confronta_archi(const void*a, const void*b);
int find(nodo_union *array_nodi, int id_nodo);
void union_rank(nodo_union *array_nodi, int root_x, int root_y,int*numCoCo);
arco **kruskal(arco**array_archi, int **cCon,int n_nodi,int n_archi, int*numCoCo);


//funzione che dato l'array di archi parsato mi ritorna la rappresentazione del grafo applicando kruskal 
//e popolando gHash, vicini e cCon, calcolando poi costoMSF e numCoCo
int hash_arco(arco *arco, int hashsize);
void inserisci_ordinato(elemento **testa,int v, int w, bool msf);
grafo crea_grafo(arco **array_archi, int n_nodi, int n_archi, int hashsize);

// calcola il numero primo che precede n
int primo_precedente(int n);
bool is_prime(int n);





#endif 