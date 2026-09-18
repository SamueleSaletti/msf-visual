Visualizzatore Concorrente MSF (Minimum Spanning Forest)
========================================================

Questo progetto visualizza in tempo reale l'aggiornamento di una Minimum Spanning Forest (MSF) su un grafo soggetto a rimozioni concorrenti di archi. Il sistema unisce algoritmi grafi multithreading in C con una dashboard interattiva web.

Architettura del Sistema
------------------------

L'applicativo è diviso in tre livelli indipendenti che comunicano tramite standard stream e WebSockets:

1.  Motore ad alte prestazioni che carica il grafo, calcola la MSF iniziale con Kruskal e gestisce una coda di operazioni concorrenti (rimozioni) smistate su più thread (Produttore-Consumatori). Invia lo stato delle operazioni tramite stringhe JSON sullo stdout.
    
2.  Un server leggero che esegue il file binario C come _Child Process_. Ascolta l'output del programma C, converte le stringhe in oggetti JSON validi e li inoltra in tempo reale ai client connessi tramite WebSocket (socket.io).
    
3.  Una pagina web passiva (zero calcoli matematici). Riceve gli eventi WebSocket e usa la libreria vis.js per manipolare il DOM del canvas, eliminando dinamicamente gli archi e ricolorando la nuova MSF quando necessario.
    

Flusso di Esecuzione: Cosa succede passo dopo passo
---------------------------------------------------

### 1\. Inizializzazione (Single Thread)

*   Il server Node.js viene avviato e si mette in ascolto. L'utente apre la pagina web e clicca su **"Carica e Inizializza MSF"**.
    
*   Node.js avvia l'eseguibile C passandogli i due file (grafo e operazioni).
    
*   Il C fa il parsing del grafo e calcola la MSF iniziale tramite l'algoritmo di **Kruskal** (con struttura dati Union-Find).
    
*   Il C stampa un JSON (init\_stats) e successivamente itera sulla sua tabella Hash per stampare tutti gli archi (graph\_data), indicando quali fanno parte della MSF.
    
*   **Sul Web:** Vis.js disegna il grafo, evidenziando in rosso gli archi della MSF.
    

### 2\. Sincronizzazione (Pausa)

*   Il programma C si blocca su una fgets() in attesa di un segnale per iniziare i thread. Questo garantisce che la pagina web abbia il tempo materiale di renderizzare migliaia di nodi prima che inizino le cancellazioni.
    
*   L'utente esamina il grafo e preme **"Avvia Threads"**.
    
*   Node.js scrive "START\\n" sullo stdin del processo C. Il C si sblocca.
    

### 3\. Esecuzione Concorrente (Multi Thread)

*   Il thread Produttore inizia a leggere il file delle operazioni (+ o -) e riempie un buffer circolare protetto da semafori.
    
*   I thread Consumatori prelevano le operazioni e tentano di cancellare gli archi operando su:
    
    *   Variabili di stato protette da Mutex e Condition Variables.
        
    *   La struttura a Grafo (Lista di adiacenze e Tabella Hash).
        
*   **Gestione Rimozione MSF:** Se un thread rimuove un arco appartenente alla MSF, acquisisce l'esclusiva sulle due componenti connesse spezzate ed esegue due **BFS** (Breadth-First Search) concorrenti rispetto agli altri thread per trovare l'arco di costo minimo che le ricollega.
    

### 4\. Aggiornamento in Tempo Reale

Ogni volta che un thread C completa un'operazione, emette una stringa JSON. Il frontend reagisce istantaneamente a questi eventi:

**Evento JSON (type)Azione del CReazione del Frontend**edge\_deletedL'arco u-v è stato eliminato dalla memoria del server.Rimuove visivamente l'arco u-v dal canvas e aggiorna le statistiche testuali.edge\_deleted (con new\_msf)La BFS ha trovato un arco sostitutivo per ricucire la MSF.Oltre a rimuovere u-v, ricolora di rosso il nuovo arco new\_msf\_u / new\_msf\_v.op\_ignored / op\_failedUn'operazione non valida o non supportata (es. +) è stata saltata.Nessuna azione visiva sul grafo.

### 5\. Termine e Calcoli Finali

*   Quando il Produttore inserisce i segnali di terminazione (T), i thread si spengono gradualmente.
    
*   Il thread principale effettua le varie pthread\_join().
    
*   Il C esegue una scansione finale (calcolo\_finale), invia le statistiche conclusive via JSON (final\_stats) e dealloca tutta la memoria (Hash, Liste, Array).
    
*   Il Frontend mostra un alert: **"Operazioni terminate!"** riportando il costo finale validato della MSF.