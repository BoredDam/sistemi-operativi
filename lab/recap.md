# Recap di laboratorio di Sistemi Operativi

Il seguente documento si pone come un riassunto degli argomenti, delle funzioni e delle chiamate a sistema, trattate nelle lezioni di laboratorio di Sistemi Operativi.

# Lavorare coi File

## File Descriptor, open() e close()

Un file descriptor è definito come un intero non-negativo associato ad un file. Per questo motivo, la funzione

```c
int open(const char *path, int oflag, [ mode_t mode ])
```

ritorna un intero. Questo sarà il file descriptor associato al file del path specificato.

Analogamente

```c
int close(int fd)
```

prende come argomento un file descriptor.

Esistono anche file descriptor di default associati già a dei canali, quali lo stdin (0), stdout (1) e lo stderr (2).


Ritornando sulla open(), esistono delle flag di apertura che possono essere combinate tra loro.

### Permessi

A un file sono associati tre possibili permessi: Read, Write e Xecute.

```c
mode_t
```

è un intero che codifica una maschera dei permessi. Ogni permesso è poi associato allo user proprietario, al gruppo proprietario e al resto degli utenti. Questo permette di specificare quali utenti godono di quali permessi. 

Ogni processo (anche la shell!) ha una maschera di default per la creazione dei file. Questa maschera di creazione, modifica quella specificata secondo un operazione di AND logico, andando a inibire alcuni permessi, per ragioni di sicurezza. *maschera-effettiva = maschera-specificata & ( ~ maschera-creazione )*.

```c
mode_t umask(mode_t cmask)
```

ritorna la maschera di creazione del processo corrente, e accetta una `mode_t cmask` per sovrascrivere quella corrente.

```c
#include "lib-misc.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#define RWRWRW_MASK (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)

int main(int argc, char *argv[]) {
    // azzera la maschera di creazione: tutti i permessi sono autorizzati
    umask(0);

    if (creat("test1.txt", RWRWRW_MASK) < 0)
        exit_with_sys_err("test1.txt");

    // inibisce la possibilità di dare permessi di lettura e scrittura al gruppo
    // e agli altri in fase di creazione di file e cartelle
    umask(S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

    if (creat("test2.txt", RWRWRW_MASK) < 0)
        exit_with_sys_err("test2.txt");

    exit(EXIT_SUCCESS);
}
```

## Accesso sequenziale e posizionamento nei file

Un file offset permette di simulare l'accesso sequenziale ai dati di un file. Per effettuare un riposizionamento, usiamo

```c
off_t lseek(int fd, off_t offset, int whence);
```

il primo argomento è il file descriptor del nostro file. l'offset (secondo argomento) indica di quanti byte spostarsi, rispetto a whence (terzo argomento). whence può essere

- `SEEK_SET`: inizio del file
- `SEEK_CUR`: rispetto alla posizione corrente
- `SEEK_END`: rispetto alla fine del file


### Lettura e scrittura da file

```c
ssize_t read(int fd, void *buf, size_t nbytes);
```

legge nbytes e li inserisce nel buffer. Ritorna il numero di byte effettivamente letti (i byte da leggere potrebbero finire prima). Ritorna -1 in caso di errore.

```c
ssize_t write(int fd, const void *buf, size_t nbytes);
```

scrive nel file nbytes contenuti nel buffer. Ritorna il numero di byte letti o -1 in caso di errore.

### Lettura e scrittura da file ATOMICA

I thread dello stesso processo condividono il file offset. Per evitare race conditions, è possibile usare le funzioni

```c
ssize_t pread(int fd, void *buf, size_t nbytes, off_t offset);
ssize_t pwrite(int fd, const void *buf, size_t nbytes, off_t offset);
```
che leggono sempre dall'inizio, e scrivono o leggono partendo dall'offset specificato, senza essere influenzate dal file offset.

### Duplicazione dei file descriptor

```c
int dup(int fd);
int dup2(int fd, int fd2);
```
dup usa la prima voce libera della tabella dei file aperti, dup2 usa la voce specificata in fd2.

## Cache del disco

```c
int fsync(int fd);
void sync(void);
```

forzano le scritture sul disco non ancora avvenute. Il Sistema Operativo usa la RAM libera come cache del disco, in fase di scrittura.