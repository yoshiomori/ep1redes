#include <stdlib.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <string.h>
#include "clients.h"

union semun {                   /* Used in calls to semctl() */
    int                 val;
    struct semid_ds *   buf;
    unsigned short *    array;
#if defined(__linux__)
    struct seminfo *    __buf;
#endif
};

Channel *init_client_manager(){
  int shmid = shmget(IPC_PRIVATE, sizeof(Channel), S_IRUSR | S_IWUSR);
  Channel *channel;
  if(shmid == -1){
    perror("shmget");
    exit(EXIT_FAILURE);
  }
  channel = (Channel*)shmat(shmid, NULL, 0);
  channel->shmid = shmid;
  channel->n = channel->clients;
  return channel;
}

/* 
 * Armazena na memória compartilhada um cliente com seu nickname,
 * host e username.
 * Retorna -1 caso exceda o numero máximo de inserções e
 * 0 se for sucesso.
 */
int insert_client(Channel *channel, char *nickname, char *host){
  Client *client = channel->n;
  /* Vendo se cabe mais um cliente */
  if(client >= channel->clients + MAXCLIENTS)
    return -1;
  /* Faz a atribuição */
  strcpy(client->nickname, nickname);
  strcpy(client->host, host);
  /* O número de clientes aumenta mais um */
  channel->n = client + 1;
  return 0;
}

Client *search_client(Channel *channel, char *nickname){
  Client *client;
  for(client = channel->clients; client < channel->n; client++)
    if(!strcmp(client->nickname, nickname))
      break;
  /* Vendo se não achou o cliente */
  if(client == channel->n)
    return (Client*) -1;
  return client;
}
