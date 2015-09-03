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


int shmid_clients, shmid_first_free, semid;
Client *clients, **first_free;

void init_client_manager(){
  shmid_clients = shmget(IPC_PRIVATE, MAXCLIENTS * sizeof(Client), S_IRWUSR);
  shmid_first_free = shmget(IPC_PRIVATE, sizeof(Client*), S_IRWUSR);
  if((shmid_first_free | shmid_clients) == -1){
    perror("shmget");
    exit(EXIT_FAILURE);
  }
  clients = (Client*)shmat(shmid_clients, NULL, 0);
  first_free = (Client**)shmat(shmid_first_free, NULL, 0);
  *first_free = clients;
}

/* 
 * Armazena na memória compartilhada um cliente com seu nickname,
 * host e username.
 * Retorna -1 caso exceda o numero máximo de inserções e
 * 0 se for sucesso.
 */
int insert_client(char *nickname, char *host){
  if(*first_free >= clients + MAXCLIENTS)
    return -1;
  strcpy((*first_free)->nickname, nickname);
  strcpy((*first_free)->host, host);
  (*first_free)++;
  return 0;
}

Client *search_client(char *nickname){
  Client *client;
  for(client = clients; client < *first_free; client++)
    if(!strcmp(client->nickname, nickname))
      break;
  if(client == *first_free)
    return (Client*) -1;
  return client;
}

/*
 * Remove o i-ésimo elemento do vetor de clientes.
 * Retorna 0 no sucesso e -1 no caso em que i está
 * fora do intervalo.
 */
int remove_client(Client *client){
  memmove(client, client + 1, (*first_free - client) * sizeof * clients);
  (*first_free)--;
  return 0;
}

int free_client_manager(){
  return shmctl(shmid_clients, IPC_RMID, NULL);
}

int nclients(){
  return *first_free - clients;
}
