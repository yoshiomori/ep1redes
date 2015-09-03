/* Por Prof. Daniel Batista <batista@ime.usp.br>
 * Em 12/08/2013
 * 
 * Um c�digo simples (n�o � o c�digo ideal, mas � o suficiente para o
 * EP) de um servidor de eco a ser usado como base para o EP1. Ele
 * recebe uma linha de um cliente e devolve a mesma linha. Teste ele
 * assim depois de compilar:
 * 
 * ./servidor 8000
 * 
 * Com este comando o servidor ficar� escutando por conex�es na porta
 * 8000 TCP (Se voc� quiser fazer o servidor escutar em uma porta
 * menor que 1024 voc� precisa ser root).
 *
 * Depois conecte no servidor via telnet. Rode em outro terminal:
 * 
 * telnet 127.0.0.1 8000
 * 
 * Escreva sequ�ncias de caracteres seguidas de ENTER. Voc� ver� que
 * o telnet exibe a mesma linha em seguida. Esta repeti��o da linha �
 * enviada pelo servidor. O servidor tamb�m exibe no terminal onde ele
 * estiver rodando as linhas enviadas pelos clientes.
 * 
 * Obs.: Voc� pode conectar no servidor remotamente tamb�m. Basta saber o
 * endere�o IP remoto da m�quina onde o servidor est� rodando e n�o
 * pode haver nenhum firewall no meio do caminho bloqueando conex�es na
 * porta escolhida.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>
#include <unistd.h>
#include "clients.h"

#define LISTENQ 1
#define MAXDATASIZE 100
#define MAXLINE 4096

int main (int argc, char **argv) {
  /* Os sockets. Um que ser� o socket que vai escutar pelas conex�es
   * e o outro que vai ser o socket espec�fico de cada conex�o */
  int listenfd, connfd;
  /* Informa��es sobre o socket (endere�o e porta) ficam nesta struct */
  struct sockaddr_in servaddr;
  /* Retorno da fun��o fork para saber quem � o processo filho e quem
   * � o processo pai */
  pid_t childpid;
  /* Armazena linhas recebidas do cliente */
  char	recvline[MAXLINE + 1], command[MAXLINE + 1], params[MAXLINE + 1], *param;
  /* Armazena o tamanho da string lida do cliente */
  ssize_t n;
  /* Iniciando o gerenciador de clientes */
  Client *client = NULL;
  char nickname[10] = "", host[100] = "", username[100] = "";
  /* Armazena o �ndice do vetor de clientes */
  int i, state = 1, back;
  int quit = 1;
  Channel *clients = init_client_manager();
  Channel *sbt = init_client_manager();
  Channel *globo = init_client_manager();
  Channel *channel;
  strcpy(sbt->name,"sbt");
  strcpy(globo->name,"globo");
   
  if (argc != 2) {
    fprintf(stderr,"Uso: %s <Porta>\n",argv[0]);
    fprintf(stderr,"Vai rodar um servidor de echo na porta <Porta> TCP\n");
    exit(1);
  }

  /* Cria��o de um socket. Eh como se fosse um descritor de arquivo. Eh
   * possivel fazer operacoes como read, write e close. Neste
   * caso o socket criado eh um socket IPv4 (por causa do AF_INET),
   * que vai usar TCP (por causa do SOCK_STREAM), j� que o IRC
   * funciona sobre TCP, e ser� usado para uma aplica��o convencional sobre
   * a Internet (por causa do n�mero 0) */
  if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("socket :(\n");
    exit(2);
  }

  /* Agora � necess�rio informar os endere�os associados a este
   * socket. � necess�rio informar o endere�o / interface e a porta,
   * pois mais adiante o socket ficar� esperando conex�es nesta porta
   * e neste(s) endere�os. Para isso � necess�rio preencher a struct
   * servaddr. � necess�rio colocar l� o tipo de socket (No nosso
   * caso AF_INET porque � IPv4), em qual endere�o / interface ser�o
   * esperadas conex�es (Neste caso em qualquer uma -- INADDR_ANY) e
   * qual a porta. Neste caso ser� a porta que foi passada como
   * argumento no shell (atoi(argv[1]))
   */
  bzero(&servaddr, sizeof(servaddr));
  servaddr.sin_family      = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port        = htons(atoi(argv[1]));
  if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1) {
    perror("bind :(\n");
    exit(3);
  }

  /* Como este c�digo � o c�digo de um servidor, o socket ser� um
   * socket passivo. Para isto � necess�rio chamar a fun��o listen
   * que define que este � um socket de servidor que ficar� esperando
   * por conex�es nos endere�os definidos na fun��o bind. */
  if (listen(listenfd, LISTENQ) == -1) {
    perror("listen :(\n");
    exit(4);
  }

  printf("[Servidor no ar. Aguardando conexoes na porta %s]\n",argv[1]);
  printf("[Para finalizar, pressione CTRL+c ou rode um kill ou killall]\n");
   
  /* O servidor no final das contas � um loop infinito de espera por
   * conex�es e processamento de cada uma individualmente */
  for (;;) {
    /* O socket inicial que foi criado � o socket que vai aguardar
     * pela conex�o na porta especificada. Mas pode ser que existam
     * diversos clientes conectando no servidor. Por isso deve-se
     * utilizar a fun��o accept. Esta fun��o vai retirar uma conex�o
     * da fila de conex�es que foram aceitas no socket listenfd e
     * vai criar um socket espec�fico para esta conex�o. O descritor
     * deste novo socket � o retorno da fun��o accept. */
    if ((connfd = accept(listenfd, (struct sockaddr *) NULL, NULL)) == -1 ) {
      perror("accept :(\n");
      exit(5);
    }
      
    /* Agora o servidor precisa tratar este cliente de forma
     * separada. Para isto � criado um processo filho usando a
     * fun��o fork. O processo vai ser uma c�pia deste. Depois da
     * fun��o fork, os dois processos (pai e filho) estar�o no mesmo
     * ponto do c�digo, mas cada um ter� um PID diferente. Assim �
     * poss�vel diferenciar o que cada processo ter� que fazer. O
     * filho tem que processar a requisi��o do cliente. O pai tem
     * que voltar no loop para continuar aceitando novas conex�es */
    /* Se o retorno da fun��o fork for zero, � porque est� no
     * processo filho. */
    if ( (childpid = fork()) == 0) {
      /**** PROCESSO FILHO ****/
      printf("[Uma conexao aberta]\n");
      /* J� que est� no processo filho, n�o precisa mais do socket
       * listenfd. S� o processo pai precisa deste socket. */
      close(listenfd);
         
      /* Agora pode ler do socket e escrever no socket. Isto tem
       * que ser feito em sincronia com o cliente. N�o faz sentido
       * ler sem ter o que ler. Ou seja, neste caso est� sendo
       * considerado que o cliente vai enviar algo para o servidor.
       * O servidor vai processar o que tiver sido enviado e vai
       * enviar uma resposta para o cliente (Que precisar� estar
       * esperando por esta resposta) 
       */

      /* ========================================================= */
      /* ========================================================= */
      /*                         EP1 IN�CIO                        */
      /* ========================================================= */
      /* ========================================================= */
      /* TODO: � esta parte do c�digo que ter� que ser modificada
       * para que este servidor consiga interpretar comandos IRC   */
      while(state){
	switch(state){
	case 1:
	  n = read(connfd, recvline, MAXLINE);
	  if(n < 0){
	    state = 0;
	    break;
	  }
	  recvline[n] = '\0';
	  printf("[Cliente conectado no processo filho %d enviou:] ",getpid());
	  if ((fputs(recvline,stdout)) == EOF) {
	    perror("fputs :( \n");
	    exit(6);
	  }
	  command[0] = '\0';
	  params[0] = '\0';
	  /* Quebrando a mensagem em tokens */
	  sscanf(recvline, "%s %s", command, params);
	  /* Interpretando os comandos */
	  if(!strcmp("NICK", command))
	    state = 2;
	  else if(!strcmp("LIST", command) && strlen(nickname))
	    state = 3;
	  else if(!strcmp("JOIN", command) && strlen(nickname))
	    state = 4;
	  else if(!strcmp("PRIVMSG", command) && strlen(nickname));
	  else if(!strcmp("DCC", command) && strlen(nickname));
	  else if(!strcmp("PART", command) && strlen(nickname))
	    state = 8;
	  else if(!strcmp("QUIT", command))
	    state = 7;
	  else{
	    /* Comando n�o v�lido */
	    if(strlen(nickname)){
	      /* Usu�rio registrado mas comando inexistente */
	      sprintf(recvline, "%s :Unknown command\n", command);
	      write(connfd, recvline, strlen(recvline));
	      break;
	    }
	    sprintf(recvline, ":You have not registered\n");
	    write(connfd, recvline, strlen(recvline));
	  }
	  break;
	case 2:/* Commando NICK */
	  n = strlen(params);
	  if(!n){
	    sprintf(recvline, ":No nickname given\n");
	    write(connfd, recvline, strlen(recvline));
	    state = 1;
	    break;
	  }
	  if(n > 10){
	    sprintf(recvline, "%s :Erroneus nickname\n", params);
	    write(connfd, recvline, strlen(recvline));
	    state = 1;
	    break;
	  }
	  client = search_client(clients, params);
	  if(client != (Client*)-1){
	    /* Mesmo cliente */
	    if(!strcmp(client->nickname, params)){
	      sprintf(recvline, "%s :Nickname is already in use\n", params);
	      write(connfd, recvline, strlen(recvline));
	    }
	    state = 1;
	    break;
	  }
	  
	  if(!strlen(nickname)){
	    /* Tratando de cliente novo */
	    /* TODO: Pegar o host name do cliente */
	    host[0] = '\0';/* Provis�rio */
	    insert_client(clients, params, host);
	    strcpy(nickname, params);
	  }
	  else {
	    /* Tratando de atualiza��o de nickname */
	    client = search_client(clients, nickname);
	    strcpy(client->nickname, params);
	    client = search_client(sbt, nickname);
	    if(client != (Client*)-1)
	      strcpy(client->nickname, params);
	    client = search_client(globo, nickname);
	    if(client != (Client*)-1)
	      strcpy(client->nickname, params);
	  }
	  state = 1;
	  break;
	case 3:
	  sprintf(recvline, "Channel :Users Name\n");
	  write(connfd, recvline, strlen(recvline));
	  sprintf(recvline, "%s\n", sbt->name);
	  write(connfd, recvline, strlen(recvline));
	  sprintf(recvline, "%s\n", globo->name);
	  write(connfd, recvline, strlen(recvline));
	  sprintf(recvline, ":End of /LIST\n");
	  write(connfd, recvline, strlen(recvline));
	  state = 1;
	  break;
	case 4:/* Trata em especial o primeiro parametro do comando JOIN */
	  if(!strlen(params)){
	    sprintf(recvline, "%s :Not enough parameters\n", command);
	    write(connfd, recvline, strlen(recvline));
	    state = 1;
	    break;
	  }
	  param = strtok(params, ",");
	  back = 5;
	  state = 6;
	  break;
	case 5:/* Trata dos outros parametros do comando JOIN */
	  param = strtok(NULL, ",");
	  if(!param){
	    state = 1;
	    break;
	  }	  
	  state = 6;
	  break;
	case 6:/* Tratamento dos parametros do JOIN */
	  if(!strcmp(sbt->name, param))
	    channel = sbt;
	  else if(!strcmp(globo->name, param))
	    channel = globo;
	  else{
	      sprintf(recvline, "%s :No such channel\n", param);
	      write(connfd, recvline, strlen(recvline));
	      state = 1;
	      break;
	  }
	  if(search_client(channel, nickname) == (Client*)-1)
	    if(insert_client(channel, nickname, host)){
	      sprintf(recvline, "%s :Cannot join channel\n", channel->name);
	      write(connfd, recvline, strlen(recvline));
	      state = 1;
	      break;
	    }
	  state = back;
	  break;
	case 7:/* QUIT */
	  client = search_client(clients, nickname);
	  remove_client(clients, client);
	  client = search_client(sbt, nickname);
	  if(client != (Client*)-1)
	    remove_client(sbt, client);
	  client = search_client(globo, nickname);
	  if(client != (Client*)-1)
	    remove_client(globo, client);
	  state = 0;
	  break;
	case 8: /* PART */
	  
	  break;
	default:
	  state = 0;
	  break;
	}
      }
      /* ========================================================= */
      /* ========================================================= */
      /*                         EP1 FIM                           */
      /* ========================================================= */
      /* ========================================================= */

      /* Ap�s ter feito toda a troca de informa��o com o cliente,
       * pode finalizar o processo filho */
      printf("[Uma conexao fechada]\n");
      exit(0);
    }
    /**** PROCESSO PAI ****/
    /* Se for o pai, a �nica coisa a ser feita � fechar o socket
     * connfd (ele � o socket do cliente espec�fico que ser� tratado
     * pelo processo filho) */
    close(connfd);
  }
  exit(0);
}
