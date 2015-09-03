#define MAXCLIENTS 1000

typedef struct _client {
  char nickname[10];
  char host[100];
} Client;

void init_client_manager();
int insert_client(char*, char*);
Client *search_client(char*);
int remove_client(Client*);
int free_client_manager();
int nclients();
