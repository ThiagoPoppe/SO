#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <signal.h>
#include <pthread.h>
#include <sys/stat.h>
#include <pwd.h>

// Definindo o número máximo de processos a serem exibidos
#define MAXPROCS 20

// Definindo variáveis globais (flags)
int gFinishedTop = 0, gKillError = 0, gUserInput = 0;

// Struct para representar nosso processo
typedef struct {
    int pid;
    char user[128];
    char cmd[128];
    char state;
} proc_t;

// Cabeçalho das funções usadas no comando top
void printCurrentTime();
void getUser(proc_t* p);
void getProcInfo(proc_t* p, const char* filepath);
void printProc(proc_t* p);

// Cabeçalho das funções de thread
void* userInputThread(void* args);

int main(int argc, const char** argv) {
    // Ponteiros para o diretório /proc e pastas do mesmo
    DIR* dirp;
    struct dirent* dp;

    // Criando a thread de input do usuário
    pthread_t inThread;
    pthread_create(&inThread, NULL, userInputThread, NULL);

    // Abrindo o diretório /proc
    if ((dirp = opendir("/proc")) == NULL) {
        fprintf(stderr, "Nao foi possivel abrir /proc");
        exit(-1);
    }

    // Loop do top (enquanto usuário não quiser sair)
    while (!gFinishedTop) {
        if (!gUserInput) {
            // Contador de processos exibidos
            int countProcs = 0; 

            // Imprimindo nome das colunas
            printCurrentTime();
            printf("  PID  |    User      |             PROCNAME           | Estado |\n");
            printf("-------|--------------|--------------------------------|--------|\n");

            // Lendo o diretório /proc
            while ((dp = readdir(dirp)) != NULL) {
                // Caso exibirmos mais do que MAXPROCS processos
                // paramos a busca antes do tempo
                if (countProcs >= MAXPROCS)
                    break;

                // Pegando apenas os processos (valores numéricos)
                if (dp->d_name[0] > '0' && dp->d_name[0] <= '9') {
                    proc_t p;
                    char filepath[32];

                    // Leitura dos dados será feita através de /proc/*/stat
                    sprintf(filepath, "/proc/%d/stat", atoi(dp->d_name));
                    
                    // Pegando os dados do processo "p"
                    getProcInfo(&p, filepath);
                    getUser(&p);

                    // Exibindo o processo
                    printProc(&p);

                    // Incrementando o número de processos exibidos
                    countProcs++;
                }
            }

            // Imprimindo mensagem de erro durante o kill
            if (gKillError) {
                printf("\nKill error: PID ou sinal inválido\n");
                gKillError = 0;
            }

            // Senão, exibimos as instruções de execução
            else {
                printf("\nPressione Enter para realizar um kill, informando o pid e sinal\n");
                printf("Caso queira retornar, basta informar pid e sinal = 0\n");
            }
            
            // Aguardando 1 segundo, reiniciando a leitura do diretório e limpando a tela
            sleep(1);
            rewinddir(dirp);
            printf("\e[1;1H\e[2J");
        }
    }

    // Fazendo join das threads
    pthread_join(inThread, NULL);
    free(dirp);
    
    return 0;
}

/***************************************************
 * Implementação das funções usadas no comando top *
 ***************************************************/

// Função que exibe a data e horário atual
void printCurrentTime() {
    time_t timer;
    char currDateTime[32];
    struct tm* tm_info;

    // Obtendo as informações
    timer = time(NULL);
    tm_info = localtime(&timer);

    // Formatando a string 
    strftime(currDateTime, 26, "%Y-%m-%d %H:%M:%S", tm_info);
    printf("Data/Horario atual: %s\n\n", currDateTime);
}

// Função que recupera o nome do usuário
void getUser(proc_t* p) {
    struct passwd* pw;
    struct stat status;
    char ss[32];

    // Obtendo o user id através de stat e
    // convertendo para o nome do usuário
    sprintf(ss, "/proc/%d", p->pid);
    stat(ss, &status);
    pw = getpwuid(status.st_uid);

    // Salvando o nome do usuário
    strcpy(p->user, pw->pw_name);
}

// Função que recupera o pid, nome do processo e estado do mesmo
void getProcInfo(proc_t* p, const char* filepath) {
    // Abrindo o arquivo em modo de leitura
    FILE* f;
    if ((f = fopen(filepath, "r")) == NULL) {
        printf("Nao foi possivel abrir %s", filepath);
        exit(-1);
    }

    // Lendo os dados e fechando o arquivo
    fscanf(f, "%d %s %c", &p->pid, p->cmd, &p->state);
    fclose(f);

    // Formatando o comando (retirando os parênteses)
    char* str = p->cmd;
    str++[strlen(p->cmd)-2] = '\0';
    strcpy(p->cmd, str);
}

// Função que imprime um processo
void printProc(proc_t* p) {
    printf("%-6d | ", p->pid);
    printf("%-12s | ", p->user);
    printf("%-30s | ", p->cmd);
    printf("%-6c |\n", p->state);
}

/***************************************
 * Implementação das funções de Thread *
 ***************************************/

// Função que irá receber o input do usuário
void* userInputThread(void* args) {
    int pid, sig;

    // Enquanto o usuário não digitar Control + D para sair
    while (getchar() != EOF) {
        // Informamos que iremos ler do input ("congelar" stdout)
        gUserInput = 1;
        
        // Lendo os valores de pid e signal
        printf("> ");
        scanf("%d %d", &pid, &sig);
        if (kill(pid, sig) < 0)
            gKillError = 1; // notificando possível erro
        
        // Limpando o buffer (possível '\n' após scanf)
        while (getchar() != '\n');
        gUserInput = 0; // notificando que não estamos recebendo nenhum sinal de entrada
    }

    // Notificando o final do programa e saindo da thread
    gFinishedTop = 1;
    pthread_exit(NULL);
}