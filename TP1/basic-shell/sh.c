#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

/* Trabalho prático feito em dupla
 * Thiago Martin Poppe
 * Giovanna Louzi Bellonia 
 */

/****************************************************************
 * Shell xv6 simplificado
 *
 * Este codigo foi adaptado do codigo do UNIX xv6 e do material do
 * curso de sistemas operacionais do MIT (6.828).
 ***************************************************************/

#define MAXARGS 10
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Definindo o tamanho máximo do nosso histórico e sua
// estrutura de dados (buffer circular)
#define MAXSIZE 50
typedef struct {
  char* cmd[MAXSIZE];
  int head, tail;
  unsigned count;
} history_t;

// Criando o histórico (variável global)
history_t* hist;

// Funções do nosso histórico (implementaçãos no final do arquivo)
history_t* create_history(void);
void destroy_history(history_t* hist);
void store_cmd(history_t* hist, const char* cmd);
void print_history(history_t* hist);

/* Todos comandos tem um tipo.  Depois de olhar para o tipo do
 * comando, o código converte um *cmd para o tipo específico de
 * comando. */
struct cmd {
  int type; /* ' ' (exec)
               '|' (pipe)
               '<' or '>' (redirection) */
};

struct execcmd {
  int type;              // ' '
  char *argv[MAXARGS];   // argumentos do comando a ser exec'utado
};

struct redircmd {
  int type;          // < ou > 
  struct cmd *cmd;   // o comando a rodar (ex.: um execcmd)
  char *file;        // o arquivo de entrada ou saída
  int mode;          // o modo no qual o arquivo deve ser aberto
  int fd;            // o número de descritor de arquivo que deve ser usado
};

struct pipecmd {
  int type;          // |
  struct cmd *left;  // lado esquerdo do pipe
  struct cmd *right; // lado direito do pipe
};

int fork1(void);  // Fork mas fechar se ocorrer erro.
struct cmd *parsecmd(char*); // Processar o linha de comando.

/* Executar comando cmd.  Nunca retorna. */
void
runcmd(struct cmd *cmd)
{
  int p[2], r;
  struct execcmd *ecmd;
  struct pipecmd *pcmd;
  struct redircmd *rcmd;

  if(cmd == 0)
    exit(0);

  switch(cmd->type){
  default:
    fprintf(stderr, "tipo de comando desconhecido\n");
    exit(-1);

  case ' ':
    ecmd = (struct execcmd*)cmd;
    if(ecmd->argv[0] == 0)
      exit(0);

    // Verificando se o comando digitado foi history.
    // Se sim, printamos o histórico de comandos
    if (strcmp(ecmd->argv[0], "history") == 0) {
      print_history(hist);
      exit(0);
    }

    /* MARK START task2
     * TAREFA2: Implemente codigo abaixo para executar
     * comandos simples. */

    // Obtendo o nome do comando
    const char* file_name = ecmd->argv[0]; 
    
    // Obtendo os argumentos (o vetor deverá terminar em NULL)
    char* args[MAXARGS+1];
    args[MAXARGS] = NULL;
    for (int i = 0; i < MAXARGS; i++)
      args[i] = ecmd->argv[i];

    // Chamando função execvp que precisa do nome do comando (file_name)
    // e do array de parâmetros terminado em NULL (args)
    execvp(file_name, args);

    /* MARK END task2 */
    break;

  case '>':
  case '<':
    rcmd = (struct redircmd*)cmd;
    /* MARK START task3
     * TAREFA3: Implemente codigo abaixo para executar
     * comando com redirecionamento. */

    // Abrindo o arquivo rcmd->file com as flags corretas
    // se for < é O_RDONLY (entrada) caso contrario é
    // O_WRONLY|O_CREAT|O_TRUNC (saída).
    // OBS.: Devemos especificar as permissões quando usamos O_CREAT.
    int fd = open(rcmd->file, rcmd->mode, 0777); // 0777 = -rwxrwxrwx
    if (fd < 0) {
      fprintf(stderr, "Erro ao abrir o arquivo\n");
      exit(-1);
    }
    
    // Mudando a entrada (rcmd->fd == 0) ou saída (rcmd->fd == 1)
    // para o file descriptor do arquivo (fd) e "desalocando" o mesmo,
    // pois seu valor já está em 0 (stdin) ou em 1 (stdout)
    dup2(fd, rcmd->fd);
    close(fd);

    /* MARK END task3 */
    runcmd(rcmd->cmd);
    break;

  case '|':
    pcmd = (struct pipecmd*)cmd;
    /* MARK START task4
     * TAREFA4: Implemente codigo abaixo para executar
     * comando com pipes. */
    
    // Realizando um pipe na variável p, tornando o
    // p[0] = p[STDIN_FILENO] = entrada dos dados e
    // p[1] = p[STDOUT_FILENO] = saída dos dados
    if (pipe(p) < 0) {
      fprintf(stderr, "Erro durante pipe\n");
      exit(-1);
    }

    // Realizando um fork (criando novo processo)
    int pid = fork();

    // Processo filho
    if (pid == 0) {
      // Fechando "canal" (file descriptor) que não será utilizado
      close(p[STDIN_FILENO]);

      // "Redirecionando" a saída para o p[1] = p[STDOUT_FILENO]
      // e chamando a função runcmd para o comando da esquerda
      dup2(p[STDOUT_FILENO], STDOUT_FILENO);
      runcmd(pcmd->left);
    }

    // Processo pai
    else if (pid > 0) {
      // Esperando o processo filho terminar
      // evitando que o mesmo fique "zombie"
      wait(&r);

      // Fechando "canal" (file descriptor) que não será utilizado
      close(p[STDOUT_FILENO]);

      // "Redirecionando" a entrada para o p[0] = p[STDIN_FILENO]
      // e chamando a função runcmd para o comando da direita
      dup2(p[STDIN_FILENO], STDIN_FILENO);
      runcmd(pcmd->right);
    }

    // Falha no fork
    else {
      fprintf(stderr, "Erro durante fork\n");
      exit(-1);
    }

    /* Na ordem acima teremos a saída do comando da esquerda servindo
     * de entrada para o comando da direita! */

    /* MARK END task4 */
    break;
  }    
  exit(0);
}

int
getcmd(char *buf, int nbuf)
{
  if (isatty(fileno(stdin)))
    fprintf(stdout, "$ ");
  memset(buf, 0, nbuf);
  fgets(buf, nbuf, stdin);
  if(buf[0] == 0) // EOF
    return -1;
  return 0;
}

int
main(void)
{
  static char buf[100];
  int r;

  // Criando o buffer circular para o histórico de comandos
  hist = create_history();
  
  // Ler e rodar comandos.
  while(getcmd(buf, sizeof(buf)) >= 0){
    /* MARK START task1 */
    /* TAREFA1: O que faz o if abaixo e por que ele é necessário?
     * Insira sua resposta no código e modifique o fprintf abaixo
     * para reportar o erro corretamente. */

    /* O if abaixo verifica se o comando digitado foi um "cd" (change directory) 
     * e posteriormente se o mesmo é válido utilizando a função chdir. */
    if(buf[0] == 'c' && buf[1] == 'd' && buf[2] == ' '){
      buf[strlen(buf)-1] = 0;
      if(chdir(buf+3) < 0)
        fprintf(stderr, "Diretorio ou arquivo nao encontrado\n");
      continue;
    }
    /* MARK END task1 */

    // Inserindo comando no histórico (sem ser quebra de linhassss)
    if (strcmp(buf, "\n"))
      store_cmd(hist, buf);

    if(fork1() == 0)
      runcmd(parsecmd(buf));        
    wait(&r);
  }

  // Desalocando o buffer circular
  destroy_history(hist);
  exit(0);
}

int
fork1(void)
{
  int pid;
  
  pid = fork();
  if(pid == -1)
    perror("fork");
  return pid;
}

/****************************************************************
 * Funcoes auxiliares para criar estruturas de comando
 ***************************************************************/

struct cmd*
execcmd(void)
{
  struct execcmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = ' ';
  return (struct cmd*)cmd;
}

struct cmd*
redircmd(struct cmd *subcmd, char *file, int type)
{
  struct redircmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = type;
  cmd->cmd = subcmd;
  cmd->file = file;
  cmd->mode = (type == '<') ?  O_RDONLY : O_WRONLY|O_CREAT|O_TRUNC;
  cmd->fd = (type == '<') ? 0 : 1;
  return (struct cmd*)cmd;
}

struct cmd*
pipecmd(struct cmd *left, struct cmd *right)
{
  struct pipecmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = '|';
  cmd->left = left;
  cmd->right = right;
  return (struct cmd*)cmd;
}

/****************************************************************
 * Processamento da linha de comando
 ***************************************************************/

char whitespace[] = " \t\r\n\v";
char symbols[] = "<|>";

int
gettoken(char **ps, char *es, char **q, char **eq)
{
  char *s;
  int ret;
  
  s = *ps;
  while(s < es && strchr(whitespace, *s))
    s++;
  if(q)
    *q = s;
  ret = *s;
  switch(*s){
  case 0:
    break;
  case '|':
  case '<':
    s++;
    break;
  case '>':
    s++;
    break;
  default:
    ret = 'a';
    while(s < es && !strchr(whitespace, *s) && !strchr(symbols, *s))
      s++;
    break;
  }
  if(eq)
    *eq = s;
  
  while(s < es && strchr(whitespace, *s))
    s++;
  *ps = s;
  return ret;
}

int
peek(char **ps, char *es, char *toks)
{
  char *s;
  
  s = *ps;
  while(s < es && strchr(whitespace, *s))
    s++;
  *ps = s;
  return *s && strchr(toks, *s);
}

struct cmd *parseline(char**, char*);
struct cmd *parsepipe(char**, char*);
struct cmd *parseexec(char**, char*);

/* Copiar os caracteres no buffer de entrada, comeando de s ate es.
 * Colocar terminador zero no final para obter um string valido. */
char 
*mkcopy(char *s, char *es)
{
  int n = es - s;
  char *c = malloc(n+1);
  assert(c);
  strncpy(c, s, n);
  c[n] = 0;
  return c;
}

struct cmd*
parsecmd(char *s)
{
  char *es;
  struct cmd *cmd;

  es = s + strlen(s);
  cmd = parseline(&s, es);
  peek(&s, es, "");
  if(s != es){
    fprintf(stderr, "leftovers: %s\n", s);
    exit(-1);
  }
  return cmd;
}

struct cmd*
parseline(char **ps, char *es)
{
  struct cmd *cmd;
  cmd = parsepipe(ps, es);
  return cmd;
}

struct cmd*
parsepipe(char **ps, char *es)
{
  struct cmd *cmd;

  cmd = parseexec(ps, es);
  if(peek(ps, es, "|")){
    gettoken(ps, es, 0, 0);
    cmd = pipecmd(cmd, parsepipe(ps, es));
  }
  return cmd;
}

struct cmd*
parseredirs(struct cmd *cmd, char **ps, char *es)
{
  int tok;
  char *q, *eq;

  while(peek(ps, es, "<>")){
    tok = gettoken(ps, es, 0, 0);
    if(gettoken(ps, es, &q, &eq) != 'a') {
      fprintf(stderr, "missing file for redirection\n");
      exit(-1);
    }
    switch(tok){
    case '<':
      cmd = redircmd(cmd, mkcopy(q, eq), '<');
      break;
    case '>':
      cmd = redircmd(cmd, mkcopy(q, eq), '>');
      break;
    }
  }
  return cmd;
}

struct cmd*
parseexec(char **ps, char *es)
{
  char *q, *eq;
  int tok, argc;
  struct execcmd *cmd;
  struct cmd *ret;
  
  ret = execcmd();
  cmd = (struct execcmd*)ret;

  argc = 0;
  ret = parseredirs(ret, ps, es);
  while(!peek(ps, es, "|")){
    if((tok=gettoken(ps, es, &q, &eq)) == 0)
      break;
    if(tok != 'a') {
      fprintf(stderr, "syntax error\n");
      exit(-1);
    }
    cmd->argv[argc] = mkcopy(q, eq);
    argc++;
    if(argc >= MAXARGS) {
      fprintf(stderr, "too many args\n");
      exit(-1);
    }
    ret = parseredirs(ret, ps, es);
  }
  cmd->argv[argc] = 0;
  return ret;
}

/****************************************************************
 * Implementação do histórico de comandos (buffer circular)
 ***************************************************************/

// Função que aloca a estrutura history_t
history_t* create_history(void) {
  history_t* hist = (history_t*) malloc(sizeof(history_t));
  hist->head = 0;
  hist->tail = 0;
  hist->count = 0;

  return hist;
}

// Função que desaloca a estrutura history_t
void destroy_history(history_t* hist) {
  for (int i = 0; i < MAXSIZE; i++)
    free(hist->cmd[i]);
  free(hist);
}

// Função que insere um comando no history_t
void store_cmd(history_t* hist, const char* cmd) {
  // Inserindo normalmente como um vetor
  if (hist->count < MAXSIZE) {
    hist->cmd[hist->tail++] = strdup(cmd);
  }

  // Inserindo como um vetor circular
  else {
    // Retirando o elemento mais antigo e colocando o mais recente
    free(hist->cmd[hist->head]);
    hist->cmd[hist->head] = strdup(cmd);

    // Atualizando os ponteiros
    hist->tail = hist->head;
    hist->head = (hist->head + 1) % MAXSIZE;
  }

  // Incrementando o contador de comandos
  hist->count++;
}

// Função que imprime o history_t
void print_history(history_t* hist) {
  // Imprimindo um vetor normalmente
  if (hist->count <= MAXSIZE) {
    for (int i = 0; i < hist->tail; i++)
      fprintf(stdout, "%d %s", i+1, hist->cmd[i]);
  }

  // Imprimindo até que os ponteiros se encontrem
  else {
    int i = hist->head;
    int cmd_count = 1;
    while (i != hist->tail) {
      fprintf(stdout, "%d %s", cmd_count, hist->cmd[i]);
      i = (i + 1) % MAXSIZE;
      cmd_count++;
    }

    // Sem essa linha o comando mais recente não é impresso
    fprintf(stdout, "%d %s", cmd_count, hist->cmd[i]);
  }
}

// vim: expandtab:ts=2:sw=2:sts=2
