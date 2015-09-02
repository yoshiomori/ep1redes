#include <stdlib.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <string.h>
#include "clients.h"

#define S_IRWUSR (S_IRUSR | S_IWUSR)

union semun {                   /* Used in calls to semctl() */
    int                 val;
    struct semid_ds *   buf;
    unsigned short *    array;
#if defined(__linux__)
    struct seminfo *    __buf;
#endif
};


int shmid_clients, shmid_first_free_pos, semid, *first_free_pos;
Clients *clients;

void init_client_manager(){
  shmid_clients = shmget(IPC_PRIVATE, MAXCLIENTS * sizeof(Clients), S_IRWUSR);
  shmid_first_free_pos = shmget(IPC_PRIVATE, sizeof(int), S_IRWUSR);
  if((shmid_first_free_pos | shmid_clients) == -1){
    perror("shmget");
    exit(EXIT_FAILURE);
  }
  first_free_pos = (int*)shmat(shmid_first_free_pos, NULL, 0);
  *first_free_pos = 0;
  clients = (Clients*)shmat(shmid_clients, NULL, 0);
}

/* 
 * Armazena na memória compartilhada um cliente com seu nickname,
 * host e username.
 * Retorna -1 caso exceda o numero máximo de inserções e
 * 0 se for sucesso.
 */
int insert_client(char *nickname, char *host){
  if(*first_free_pos >= MAXCLIENTS || !nickname || !host)
    return -1;
  strcpy(clients[*first_free_pos].nickname, nickname);
  strcpy(clients[(*first_free_pos)++].host, host);
  return 0;
}

Clients *search_client(char *nickname, int *i){
  if(!nickname || !i)
    return (Clients*) -1;
  for(*i = 0; *i < *first_free_pos; (*i)++)
    if(!strcmp(clients[*i].nickname, nickname))
      break;
  if(*i == *first_free_pos)
    return (Clients*) -1;
  return clients + *i;
}

/*
 * Remove o i-ésimo elemento do vetor de clientes.
 * Retorna 0 no sucesso e -1 no caso em que i está
 * fora do intervalo.
 */
int remove_client(int i){
  if(i < 0 || i >= MAXCLIENTS)
    return -1;
  memmove(clients + i, clients + i + 1, (*first_free_pos - i) * sizeof * clients);
  (*first_free_pos)--;
  return 0;
}

int free_client_manager(){
  return shmctl(shmid_clients, IPC_RMID, NULL);
}

int nclients(){
  return *first_free_pos;
}
