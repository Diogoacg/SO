#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <ctype.h>

#define SERVER_PIPE "client_server_fifo"
#define CLIENT_PIPE "server_client_fifo"

#define MAX_COMANDOS 10

typedef struct {
    char *tipo_execucao;
    char *flag;
    char *comandos[MAX_COMANDOS];
    int num_comandos;
} Argumentos;

void analisar_argumentos(int argc, char *argv[], Argumentos *args) {
    if (argc < 4) {
        printf("Erro: Número insuficiente de argumentos\n");
        exit(1);
    }

    args->tipo_execucao = argv[1];
    args->flag = argv[2];
    args->num_comandos = 0;

    if (argv[3] != NULL) {
        char *comandos = strdup(argv[3]);
        char *token = strtok(comandos, "|");
        while (token && args->num_comandos < MAX_COMANDOS) {
            args->comandos[args->num_comandos] = token;
            args->num_comandos++;
            token = strtok(NULL, "|");
        }
    }
}

#define MAX_ARGV 64

char **parse_command( char *cmd) {
    char **argv = malloc(MAX_ARGV * sizeof(char *));
    size_t argc = 0;

    while (1) {
        while (isspace(*cmd)) ++cmd;

        if (*cmd == '\0') break;

        argv[argc++] = cmd;
        while (*cmd && !isspace(*cmd)) ++cmd;

        if (*cmd == '\0') break;

        *(char *) cmd++ = '\0';
        if (argc >= MAX_ARGV - 1) break;
    }
    argv[argc] = NULL;

    return argv;
}

void execute_programs(size_t n, char *commands[],int fd) {
    pid_t pids[n];
    char **argvs[n];
    
    size_t i;

    pid_t pipelineID = getpid();

    struct timeval start_time;
    gettimeofday(&start_time, NULL);

    printf("Pipeline iniciada com PID %d\n",  pipelineID);

        char msg[1024];
        sprintf(msg, "EXEC %d %ld\n",  pipelineID, start_time.tv_sec);
        if (write(fd, msg, strlen(msg)) < 0) {
            perror("Erro ao enviar mensagem para o servidor");
        }
    // Cria os processos filhos para cada comando
    for (i = 0; i < n; i++) {
        argvs[i] = parse_command(commands[i]);
        pid_t pid = fork();

        if (pid == 0) { // Processo filho
            execvp(argvs[i][0], argvs[i]);
            perror("execvp");  // Execução retornará somente em caso de erro
            _exit(1);
        } else if (pid > 0) { // Processo pai
            pids[i] = pid;
        }

    }

        // Espera todos os processos filhos terminarem
    for (i = 0; i < n; i++) {
        int status;
        waitpid(pids[i], &status, 0);
        free(argvs[i]);
    }
    struct timeval end_time;
    gettimeofday(&end_time, NULL);

    long elapsed_time = (end_time.tv_sec - start_time.tv_sec) * 1000 + (end_time.tv_usec - start_time.tv_usec) / 1000;

    printf("Pipeline terminada com PID %d\n", pipelineID);
    printf("Tempo de execução: %ldms\n", elapsed_time);

    sprintf(msg, "END %d %ld\n", pipelineID, end_time.tv_sec);
    if (write(fd, msg, strlen(msg)) < 0) {
        perror("Erro ao enviar mensagem para o servidor");
    }

}
int executar_comando(char *nome_comando, int fd){

    char **argvs = parse_command(nome_comando);

    pid_t pid = fork();
    if (pid == -1) {
        perror("Erro ao criar o processo filho");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // processo filho executa o programa
        execvp(argvs[0], argvs);
        perror("Erro ao executar o programa");
        exit(EXIT_FAILURE);
    } else {
        // processo pai notifica o servidor e o utilizador
        struct timeval start_time;
        gettimeofday(&start_time, NULL);

        printf("Programa %s iniciado com PID %d\n", nome_comando, pid);

        char msg[1024];
        sprintf(msg, "EXEC %d %s %ld\n", pid, nome_comando, start_time.tv_sec);
        if (write(fd, msg, strlen(msg)) < 0) {
            perror("Erro ao enviar mensagem para o servidor");
        }

        int status;
        waitpid(pid, &status, 0);

        struct timeval end_time;
        gettimeofday(&end_time, NULL);

        long elapsed_time = (end_time.tv_sec - start_time.tv_sec) * 1000 + (end_time.tv_usec - start_time.tv_usec) / 1000;

        printf("Programa %s terminado com PID %d\n", nome_comando, pid);
        printf("Tempo de execução: %ldms\n", elapsed_time);

        sprintf(msg, "END %d %ld\n", pid, end_time.tv_sec);
        if (write(fd, msg, strlen(msg)) < 0) {
            perror("Erro ao enviar mensagem para o servidor");
        }
    }
    return 0;

}


int main(int argc, char *argv[]) {
    int i = 0;

    Argumentos args;

    analisar_argumentos(argc, argv, &args);

    printf("Tipo de execucao: %s\n", args.tipo_execucao);
    printf("Flag: %s\n", args.flag);
    printf("Numero de comandos: %d\n", args.num_comandos);
    for (int i = 0; i < args.num_comandos; i++) {
        printf("Comando %d: %s\n", i + 1, args.comandos[i]);
    }

    int fd;
    if ((fd = open(SERVER_PIPE, O_WRONLY)) < 0) {
        perror("Erro ao abrir o pipe");
        exit(EXIT_FAILURE);
    }
    executar_comando(args.comandos[0],fd);
    execute_programs(args.num_comandos, args.comandos, fd);
    exit(EXIT_SUCCESS);
}