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

                array_archi[i_corrente] = arco_temp;
                i_corrente++;

            }
            
        }
    }

    //questo perchè la getline lo alloca dinamicamente
    if(buffer!=NULL) free(buffer);
    return array_archi;
}

