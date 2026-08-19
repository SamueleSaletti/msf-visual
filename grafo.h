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


typedef struct {
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


arco **parse_file(FILE *f, int *nodi, int *archi);

// ALGORITMO DI KRUSKAL, dato il grafo ritorna la msf usando una disjoint-set ds per trovare se due nodi fanno parte dello stesso albero (e individuare i cicli)


// calcola il numero primo che precede n
int primo_precedente(int n);
bool is_prime(int n);





#endif 