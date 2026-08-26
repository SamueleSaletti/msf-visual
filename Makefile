CC = gcc
CFLAGS = -std=c11 -Wall -g -O3 
LDLIBS = -pthread

SORGENTI = main.c grafo.c xerrori.c
#sostituisco i nomi dei sorgenti con il loro corrispettivo in file oggetto
OGGETTI = $(SORGENTI:.c=.o) 

all: msf.out

msf.out: $(OGGETTI)
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@ $(LDLIBS)

grafo.o: grafo.h xerrori.h
main.o: grafo.h xerrori.h
xerrori.o: xerrori.h
 

.PHONY: all clean 

clean:
	rm -f $(OGGETTI) msf.out 