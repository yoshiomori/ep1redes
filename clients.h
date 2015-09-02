#define MAXCLIENTS 1000

typedef struct _clients {
  char nickname[10];
  char host[100];
  char username[100];
} Clients;

void init_client_manager();
int insert_client(char*, char*);
Clients *search_client(char*, int*);
int remove_client(int);
int free_client_manager();
int nclients();
