# Soluzioni per la mutua esclusione

La mutua esclusione è fondamentale per regolare l'accesso alle aree critiche da parte dei vari flussi di esecuzione di processi o thread: il fine ultimo, è quello di evitare le corse critiche, note anche come race condition.

### Regole per una buona soluzione alle race condition

1. Mutua esclusione - *ovviamente*.
2. Nessuna assunzione su velocità e numero di CPU - *deve essere valida su qualsiasi sistema*.
3. Nessun processo fuori dalla propria sezione critica può bloccare altri processi - *non ottimale*.
4. Nessun processo può stare all'infinito in attesa d'entrare nella propria sezione critica.

## 1 - Inibizione degli interrupt

Questa soluzione funziona solo nei contesti a singolo processore.

Gli interrupt sono la condizione principale per cui un processo $P_2$ potrebbe intervenire durante l'esecuzione di un'area critica da parte di un processo $P_1$. In particolar modo, ci riferiamo all'interrupt del clock hardware di sistema, che si occupa della prelazione.

Pro di questa soluzione:
- È semplice. `enter_region()` ed `exit_region()` delimitano le aree critiche. La prima disabilita gli interrupt, la seconda li riabilita.

- Disabilitare la prelazione garantisce che non ci sia modo alcuno di interrompere il processo nella sua area critica, minimizzando i tempi di uscita da quest'ultima

Contro di questa soluzione:
- Non funziona nei sistemi multiprocessore: inibire gli interrupt di una CPU influenza solo ed esclusivamente quella CPU.

- Disabilitare gli interrupt è generalmente un'operazione rischiosa, e soprattutto una grande responsabilità affidata a dei programmatori inaffidabili 💀.

- In particolar modo, gli interrupt devono essere gestiti piuttosto tempestivamente, in quanto il loro accumularsi potrebbe causare la perdita di questi (tra cui segnali da dispositivi di I/O).


A livello Kernel. I programmatori del Sistema Operativo hanno modo di valutare quando disabilitare gli interrupt può essere una buona soluzone di basso livello, e soprattutto sicura.


## 2 - Variabili di lock

Non funziona. 

Una variabile di lock con valore 1 se la risorsa è in uso, e 0 se è libera. Può causare race-condition sulla variabile stessa, dando risultati anomali. Non è una soluzione valida.

Fa inoltre uso di busy-waiting, consumando inutilmente tempo nella CPU.

## 3 - Alternanza stretta

Funziona, ma è troppo rigida e viola una delle condizioni poste.

Si assegna un ID ad ogni processo.

- `enter_region(int process)` verifica una variabile `turn`: se questa coincide con l'ID del processo che vuole entrare nella sezione critica, va tutto bene, altrimenti aspetta (busy waiting).

- `leave_region(int process)` una sola istruzione: assegna alla variabile `turn` il valore del prossimo processo.

```c
int N = 2;
int turn;

void enter_region(int process) {
    while (turn != process) {
        /*do nothing*/
    }
}

void exit_region(int process) {
    turn = 1 - process /*passa il testimone*/
}
```

Questa soluzione funziona, ma viola la condizione tre per la realizzazione di una soluzione valida alle race condition. In generale, è veramente troppo rigida.

## 4 - Soluzione di Peterson

È stata invalidata dai sistemi multicore moderni che riordinano gli accessi in memoria. È inoltre, scalare la soluzione a più di due processi aumenta la complessità della soluzione.
Tanto per cambiare, fa uso di busy waiting.

- `enter_region(int process)`: il processo si dichiara interessato nel proprio slot dell'array, e imposta `turn = process`. Se più processi sono interessati, solo quello che è riuscito a impostare `turn == process`

- `exit_region(int process)`: il processo si dichiara non interessato.



```c
int N = 2; // numero di processi
int turn;
bool interested[N];

void enter_region(int process) {
    other = 1 - process;
    interested[process] = true;
    turn = process
    while (interested[other] == true && turn == process) { 
        /* se la prima condizione è true, la seconda condizione è 
        fondamentale per capire chi ha vinto la "contesa" della zona critica */

        /* also, do nothing */
    }
}

void exit_region(int process) {
    interested[process] = false;
}
```
Pro.
- Non rigida.

Contro.
- Non funziona nei sistei moderni.
- Difficilmente scalabile.
- Busy waiting.

## 5 - TSL o XCHG

Si introduce una nuova istruzione atomica, Test-And-Set-Lock.

La TSL consiste in:

```assembly
MOV REGISTER, LOCK
MOV LOCK, #1
```
ossia, due istruzioni atomiche. La TSL blocca temporaneamente l'accesso al bus in memoria durante l'operazione di fetch e store, in modo che la prima `MOV` non venga eseguita da più processi. 

La nostra soluzione per la concorrenza, usa la TSL nel seguente modo.

```assembly
enter_region:
    TSL REGISTER, LOCK
    CMP REGISTER, #0
    JNE enter_region
    RET

leave_region:
    MOV LOCK, #0
    RET
```

Infatti, la TSL è solo un'istruzione che viene usata nella nostra soluzione, e non è la soluzione vera e propria.

In altre architetture (Intel x86), un'istruzione equivalente è la `XCHG`.

```assembly
enter_region:
    MOV REGISTER, #1
    XCHG REGISTER, LOCK
    RET

leave_region:
    MOV LOCK, #0
    RET
```

Questa soluzione può essere usata anche a livello utente, ma si predilige, nei sistemi multi-core, a operazioni a livello kernel.

Nei sistemi mono-core, si usa l'inibizione degli interrupt (ma sempre e solo a livello kernel...).

## 6 - Sleep e Wake-up

Per evitare lo spreco di CPU causato dal busy-waiting, ma anche il problema dell'inversione di priorità, l'OS offre primitive di comunicazione tra processi che permettono di portare un processo nello stato `blocked`, invece che di portarlo in una attesa attiva.

Un esempio sono proprio le istruzioni `sleep` e `wakeup`.

- `sleep` sospende l'esecuzione del chiamante. Questo potrà tornare ad avere la CPU solo quando sarà risvegliato. Manderà il processo nello stato `blocked`.

- `wakeup` sveglia un processo, facendolo tornare nello stato `ready`.

## 7 - Semafori

I semafori rappresentano un'evoluzione del meccanismo di sincronizzazione di `sleep` `wakeup`. Rimuovono busy-waiting, e offrono sincronizzazione.

Consistono in una variabile intera unsigned condivisa, che offre due operazioni fondamentali:

- `up(S)`: incrementa il valore di `S`.
- `down(S)`: decrementa il valore di `S`. Se `S=0`, la `down(S)` diventa bloccante.

L'uso di `down` e `up` non genera race condition sulla variabile `S` in quanto le operazioni sono atomiche. L'atomicità è ottenuta tramite:
- in sistemi single-core -> disabilitazione degli interrupt.
- in sistemi multi-core -> istruzioni atomiche `TSL` e `XCHG`.

I semafori sono implementati, solitamente, senza busy waiting. Ciò viene fatto tramite una lista di processi bloccati.

## 7.1 - Semafori binari - Mutex

I semafori binari assumono valori 0-1. Versione semplificata del semaforo (binario) che gestisce mutua esclusione, senza bisogno di contare le risorse disponibili. 

## 7.2 - Futex

Alcuni sistemi operativi supportano dei mutex livello utente (Fast User-Space Mutex).

I futex gestiscono la situazione, per quanto possibile, in modalità utente. La contesa avviene in modalità utente con le istruzioni TSL/XCHG, e il kernel interviene solo in caso di bloccaggio.

Si prende quindi il meglio dello spazio utente e dello spazio kernel:

- Lo spazio utente gestisce la contesa delle zone critiche. Non si chiama il kernel per verificare se una zona critica è occupata.
- Il kernel blocca e risveglia thread, e gestisce una coda di thread bloccati.

La libreria `pthreads` implementa i mutex proprio in questo modo.


## 8 - Monitor

È un costrutto di alto livello offerto da alcuni linguaggi. 
È un tipo di dato astratto, contenente dati e metodi, che offre dei vincoli sui dati. Le specifiche sul monitor sono relative al linguaggio, ed è il compilatore (o interprete) a renderle tali. 

Il programmatore (che può commettere errori), tramite i monitor delega al compilatore (o interprete) la responsabilità di usare le primitive di sincronizzazione sottostanti.

- Le strutture dati del monitor sono private.
- I metodi interni al monitor sono sempre eseguiti in mutua esclusione.

Un thread che sta eseguendo un metodo interno al monitor, è detto "dentro al monitor".

I monitor offrono pure la possibilità di gestire delle **variabili condizione**: queste permettono di gestire e sicronizzare degli eventi, tramite opportuni metodi `wait(cond)`, `signal(cond)`.

- `wait(cond)`: il processo viene inserito in una coda associata all'evento `cond`.
- `signal(cond)`: rende attivo un processo della coda associata all'evento `cond`. Non ha effetti se questa è vuota.

### 8.1 Semantiche signal

| Semantica                      | Chi continua dopo `signal()`?          | Quando il segnalato riprende?                             | Chi ha la mutua esclusione subito dopo? | Comportamento del monitor              |
| ------------------------------ | -------------------------------------- | --------------------------------------------------------- | --------------------------------------- | -------------------------------------- |
| **Hoare (Signal-and-Wait)**    | Il **segnalato** (chi era in `wait()`) | Subito dopo il `signal()`                                 | Il segnalato                            | Il segnalante si sospende              |
| **Mesa (Signal-and-Continue)** | Il **segnalante**                      | Dopo che il segnalante **esce dal monitor**               | Il segnalante                           | Il segnalato va in coda di attesa      |
| **Pascal**                     | Il **segnalante**                      | Dopo che **tutti i thread** attivi **escono dal monitor** | Il segnalante                           | Il segnalato **ritarda ancora di più** |




## 9 - Scambio di messaggi
Non è un aprroccio di sincronizzazione, tanto più di IPC.
È un approccio basato su messaggi: i processi si scambiano messaggi tramite due primitive:

- `receive(sorgente, messaggio)`: è una chiamata bloccante fino all'arrivo del messaggio.
- `send(destinazione, messaggio)`: può essere bloccante se il messaggio non viene ricevuto, quando non si utilizzanoun buffer.

Sono approcci molto costosi.

## 9.1 Rendezvous

Strategia senza buffer.

## 9.2 Mailbox

Usa un buffer per messaggi: quando il buffer è pieno, il processo che esegue send viene bloccato fino a quando non c'è posto.


## Recap

- Inibire gli interrupt: funziona in sistemi monoprocessore.

Soluzioni con busy waiting -> spin lock:

- Variabile di lock: non funziona.
- Alternanza stretta: funziona, ma è rigida e viola la condizione 3.
- Soluzione di Peterson: nei sistemi moderni non funziona.
- TSL e XCHG: ottima soluzione a livello kernel, fa ancora uso di busy waiting.

Soluzioni senza busy waiting:

- `sleep` e `wakeup`: rispettivamente, addormentano un processo portandolo nello stato blocked, o lo svegliano portandolo nello stato ready.
- Semafori con `down` e `up`, tra cui:
    - Semafori numerici per contare.
    - Mutex, per sincronizzare.
        - Futex, per un'implementazione migliore (*Fast User-Space Mutex*).
- Monitor, gestione della sincronizzazione e della concorrenza svolta dal compilatore/interprete.
- Variabili condizionali per migliorare la sincronia.
- Approccio orientato ai messaggi, con strategia Rendezvous o Mailbox. 