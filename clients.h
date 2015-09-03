#define MAXCLIENTS 1000
#define remove_client(channel, client) (memmove((client), (client) + 1, (((channel)->n--) - (client)) * sizeof(Client)))
#define free_client_manager(channel) (shmctl((channel)->shmid, IPC_RMID, NULL))
#define nclients(channel) ((int)((channel)->n - (channel)->clients))

typedef struct _client {
  char nickname[10];
  char host[100];
} Client;

typedef struct _channel {
  int shmid;
  char name[100];
  Client clients[MAXCLIENTS];
  Client *n;
} Channel;

Channel *init_client_manager();
int insert_client(Channel*, char*, char*);
Client *search_client(Channel*, char*);
