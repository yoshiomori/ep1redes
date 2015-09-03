#define MAXCLIENTS 1000

typedef struct _client {
  char nickname[10];
  char host[100];
  char username[100];
} Client;

void init_client_manager();
int insert_client(char*, char*);
Client *search_client(char*, int*);
int remove_client(int);
int free_client_manager();
int nclients();
