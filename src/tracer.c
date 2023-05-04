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
#include <sys/wait.h>
#include <stdbool.h>
#include <dirent.h>

#define SERVER_PIPE "../tmp/client_server_fifo"
#define CLIENT_PIPE_PREFIX "../tmp/server_client_"

#define MAX_BUF 1024
#define MAX_COMANDOS 10
#define MAX_ARGV 64

typedef struct {
    char *tipo_execucao;
    char *flag;
    char *comandos[MAX_COMANDOS];
    int num_comandos;
} Argumentos;

typedef struct
{
    int pid;
    char nome_comando[10][1024];
    int num_comandos;
    long timestamp;
    char status[5];
    char exec_type[16];
    long exec_elapsed_time;
    char client_pipe[64];
    char command[16];
} InfoPipe;

void analisar_argumentos(int argc, char *argv[], Argumentos *args) {
    // Verifica se o número mínimo de argumentos foi fornecido (4)
    if (argc < 4) {
        perror("ERROR: Invalid number of arguments");
        return;
    }

    // Atribui os argumentos à estrutura 'args'
    args->tipo_execucao = argv[1];
    args->flag = argv[2];
    args->num_comandos = 0;

    // Verifica se o quarto argumento (lista de comandos) está presente
    if (argv[3] != NULL) {
        char *comandos = strdup(argv[3]);
        char *token = strtok(comandos, "|");
        
        // Itera pelos tokens e armazena cada comando na estrutura 'args'
        while (token && args->num_comandos < MAX_COMANDOS) {
            args->comandos[args->num_comandos] = token;
            args->num_comandos++;
            token = strtok(NULL, "|");
        }
    }
}

// Função que analisa uma string de comando e extrai os argumentos individuais
char **parse_command(char *cmd) {
    // Aloca espaço para armazenar os argumentos extraídos
    char **argv = malloc(MAX_ARGV * sizeof(char *));
    size_t argc = 0;

    // Continua a análise enquanto não encontrar o caractere NULL (fim da string)
    while (1) {
        // Avança pelos espaços em branco até encontrar um caractere diferente
        while (isspace(*cmd)) ++cmd;

        if (*cmd == '\0') break;
        argv[argc++] = cmd;
        
        // Avança pelo conteúdo do argumento
        while (*cmd && !isspace(*cmd)) ++cmd;

        // Se encontrou o fim da string, encerra o loop
        if (*cmd == '\0') break;

        // Substitui o espaço em branco por um caractere NULL e avança para o próximo caractere
        *(char *) cmd++ = '\0';

        // Se atingiu o limite máximo de argumentos, encerra o loop
        if (argc >= MAX_ARGV - 1) break;
    }

    argv[argc] = NULL;
    return argv;
}

//envia informação para o servidor
void enviar_informacao(int fd, InfoPipe *info) {
    if (write(fd, info, sizeof(InfoPipe)) < 0) {
        perror("ERROR: writing to pipe");
        return;
    }
}

void pipeline_aux(size_t n, char *commands[], int fd) {

    if (n == 0) {
        return;
    }

    int pipefd[2];
    pipe(pipefd);

    pid_t pid = fork();

    if (pid == -1) {
        perror("ERROR: fork failed");
        return;
    } else if (pid == 0) {         // Child process
        close(pipefd[0]);          // Fecha o lado de leitura do pipe
        dup2(fd, STDIN_FILENO);    // Duplica o descritor de arquivo para o stdin
        if (n > 1) {
            dup2(pipefd[1], STDOUT_FILENO); // Duplicando o descritor de arquivo para o stdout
        }
        close(pipefd[1]);
        
        char **argvs = parse_command(commands[0]);
        execvp(argvs[0], argvs);

        perror("ERROR: execvp failed");
        return;
    } else { // Parent process
        close(pipefd[1]); // Fecha o lado de escrita do pipe
        
        int status;
        waitpid(pid, &status, 0); // Espera o processo filho terminar

        if (n > 1) {
            pipeline_aux(n - 1, &commands[1], pipefd[0]); // Executa os próximos comandos em "pipeline"
        }
        
        close(pipefd[0]);
    }
}

int executarPipeline(size_t n, char *comandos[], int fd) {

    char mensagem[MAX_BUF];

    // Armazena o tempo de início do pipeline
    struct timeval start_time;
    gettimeofday(&start_time, NULL);
    pid_t pid = getpid();

    sprintf(mensagem, "Running PID %d\n", pid);
    write(1, mensagem, strlen(mensagem));

    InfoPipe info;

    // Inicializa a estrutura info com as informações do pipeline
    strcpy(info.exec_type, "EXECUTE");
    info.pid = pid;
    info.num_comandos = n;
    info.timestamp = start_time.tv_sec;
    strcpy(info.status, "EXEC");

    // Processa cada comando no pipeline
    for (int i = 0; i < info.num_comandos; i++) {
        // abrir um novo bloco
        {
            char temp[10];
            strcpy(temp, comandos[i]);
            char** argv = parse_command(temp);
            strncpy(info.nome_comando[i], argv[0], sizeof(info.nome_comando[i]));
        }
        // temp é destruido por causa do bloco
    }   

    // Envia as informações do pipeline para o monitor através do descritor de arquivo fd
    enviar_informacao(fd, &info);

    // Executa os comandos do pipeline
    pipeline_aux(info.num_comandos, comandos, fd);

    // Armazena o tempo de término do pipeline e calcula o tempo decorrido
    struct timeval end_time;
    gettimeofday(&end_time, NULL);
    long elapsed_time = (end_time.tv_sec - start_time.tv_sec) * 1000 + (end_time.tv_usec - start_time.tv_usec) / 1000;

    // Exibe o tempo de execução
    sprintf(mensagem, "Ended in %ld ms\n", elapsed_time);
    write(1, mensagem, strlen(mensagem));

    // Atualiza o status da estrutura InfoPipe e envia as informações finais
    strcpy(info.status, "END");
    info.exec_elapsed_time = elapsed_time;
    enviar_informacao(fd, &info);

    return 0;
}



int executar_comando(char *nome_comando, int fd) {
    char mensagem[MAX_BUF];
    int pipefd[2];

    if (pipe(pipefd) == -1) { // Adicionado: criação do pipe
        perror("ERROR: pipe failed");
        exit(EXIT_FAILURE);
    }
    // Converte o nome do comando em um vetor de argumentos
    char **argvs = parse_command(nome_comando);

    // Armazena o tempo de início do programa
    struct timeval start_time;
    gettimeofday(&start_time, NULL);
    pid_t pid = getpid();

    

    InfoPipe info;

    // Inicializa a estrutura info com as informações do comando
    strcpy(info.exec_type, "EXECUTE");
    strncpy(info.nome_comando[0], nome_comando, sizeof(info.nome_comando[0]));
    info.num_comandos = 1;
    info.timestamp = start_time.tv_sec;
    strcpy(info.status, "EXEC");

    pid_t dadorson = fork();

    if (dadorson == -1) {
        perror("ERROR: fork failed");
        exit(EXIT_FAILURE);
    } else {

        if (dadorson == 0) {
            pid = getpid();
            info.pid = pid;

            //comunicar com o processo pai
            close(pipefd[0]);
            write(pipefd[1], &info, sizeof(InfoPipe));
            close(pipefd[1]);

            // Exibe o PID do processo filho
            sprintf(mensagem, "Running PID %d\n", pid);
            write(1, mensagem, strlen(mensagem));
            // Envia as informações do comando para o monitor através do descritor de arquivo fd
            enviar_informacao(fd, &info);
            // Processo filho executa o programa
            execvp(argvs[0], argvs);
            perror("ERROR: execvp failed");
            exit(EXIT_FAILURE);
            
        } else {
            int status;

            // Processo pai aguarda o término do processo filho
            waitpid(dadorson, &status, 0);

            // Armazena o tempo de término do programa e calcula o tempo decorrido
            struct timeval end_time;
            gettimeofday(&end_time, NULL);
            long elapsed_time = (end_time.tv_sec - start_time.tv_sec) * 1000 + (end_time.tv_usec - start_time.tv_usec) / 1000;

            // Exibe o tempo de execução
            sprintf(mensagem, "Ended in %ld ms\n", elapsed_time);
            write(1, mensagem, strlen(mensagem));

            close(pipefd[1]);
            while(read(pipefd[0], &info, sizeof(InfoPipe)) > 0) {
                // Espera até que o processo filho envie as informações do comando
            }

            close(pipefd[0]);
            // Atualiza o status da estrutura InfoPipe e envia as informações finais
            info.exec_elapsed_time = elapsed_time;
            info.timestamp = end_time.tv_sec;
            strcpy(info.status, "END");

            enviar_informacao(fd, &info);
        }
    }

    return 0;
}


void print_running_pids_from_server(int fd) {

    // Inicializa a estrutura para usar com a função select()
    fd_set read_fds;
    struct timeval timeout;
    int select_result;

    InfoPipe info;
    char mensagem[MAX_BUF];

    while (1) {

        // Limpa e configura o conjunto de descritores de arquivo
        FD_ZERO(&read_fds);
        FD_SET(fd, &read_fds);

        // Configura o intervalo de tempo limite para 1 segundo
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        // Aguarda a atividade no descritor de arquivo selecionado
        select_result = select(fd + 1, &read_fds, NULL, NULL, &timeout);

        if (select_result > 0 && FD_ISSET(fd, &read_fds)) {

            // Lê a informação do pipe
            if (read(fd, &info, sizeof(InfoPipe)) < 0) {
                perror("ERROR: read failed");
                return;
            }

            // Exibe o PID e os comandos executados
            sprintf(mensagem, "%d ", info.pid);
            write(1, mensagem, strlen(mensagem));
            for (int i = 0; i < info.num_comandos; i++) {
                if (i == 0) {
                    snprintf(mensagem, sizeof(mensagem)+1,"%s ", info.nome_comando[i]);
                } else {
                    snprintf(mensagem, sizeof(mensagem)+3 ,"| %s ", info.nome_comando[i]);
                }
                write(1, mensagem, strlen(mensagem));
            }

            // Exibe o tempo decorrido
            sprintf(mensagem, "%ldms\n", info.exec_elapsed_time);
            write(1, mensagem, strlen(mensagem));

        } else {
            // Termina a função se não houver mais dados a serem lidos
            break;
        }
    }
}

// Cria um pipe nomeado (FIFO) para o cliente e retorna o descritor de arquivo
int create_client_fifo(char *client_fifo_path) {
    sprintf(client_fifo_path, "%s%d", CLIENT_PIPE_PREFIX, getpid());

    if (mkfifo(client_fifo_path, 0666) == -1) {
        if (errno != EEXIST) {
            perror("ERROR: mkfifo failed");
            exit(EXIT_FAILURE);
        }
    }
    return 1;
}



int read_from_pipe_and_print(const char *pipe_name) {
    int pipe_fd;


    // Abre o pipe para leitura
    pipe_fd = open(pipe_name, O_RDONLY);
    if (pipe_fd < 0) {
        perror("ERROR: open failed");
        exit(EXIT_FAILURE);
    }

    // Lê do pipe e imprime com write usando um loop
    ssize_t bytes_read;
    char buffer[MAX_BUF];

    while ((bytes_read = read(pipe_fd, buffer, MAX_BUF - 1)) > 0) {
        buffer[bytes_read] = '\0';

        // Imprimir o conteúdo do buffer usando write
        ssize_t bytes_written = write(STDOUT_FILENO, buffer, bytes_read);
        if (bytes_written != bytes_read) {
            perror("ERROR: write failed");
            close(pipe_fd);
            exit(EXIT_FAILURE);
        }
    }

    // Fecha o descritor de arquivo do pipe
    close(pipe_fd);

    return 0;
}

// Função principal do cliente que recebe os argumentos da linha de comando e envia para o servidor
int main(int argc, char *argv[]) {

    char client_fifo_path[MAX_BUF];
    create_client_fifo(client_fifo_path);
    
    int fd;
    int fd_client;
    
    // Abre o pipe do servidor para escrita
    if ((fd = open(SERVER_PIPE, O_WRONLY)) < 0) {
        perror("ERROR: open failed");
        exit(EXIT_FAILURE);
    }

    // Abre o pipe do cliente para leitura
    if ((fd_client = open(client_fifo_path, O_RDONLY|O_NONBLOCK)) < 0) {
        perror("ERROR: open failed");
        exit(EXIT_FAILURE);
    }
    
    
    // Verifica se o comando é para executar um ou uma pipeline de comandos
    if(strcmp(argv[1],"execute")==0){
        Argumentos args;

        analisar_argumentos(argc, argv, &args);
        // Verifica se a flag é -u ou -p
        if (strcmp(args.flag, "-u") == 0) {
            // Executa o comando
            executar_comando(args.comandos[0], fd);

        } else if (strcmp(args.flag, "-p") == 0) {
            // Executa a pipeline de comandos
            executarPipeline(args.num_comandos, args.comandos, fd);
        }
    }
    else if (strcmp(argv[1], "status") == 0) {

        // Cria a estrutura de dados para enviar ao servidor
        InfoPipe info;
        InfoPipe infoFromServer;

        strcpy(info.client_pipe , client_fifo_path);
        strcpy(info.exec_type, "STATUS");

        // Envia a informação para o servidor
        enviar_informacao(fd, &info);
        while(read(fd_client, &infoFromServer, sizeof(infoFromServer)) < 0){
            // Espera até que o servidor envie a informação
        }
        print_running_pids_from_server(fd_client);
    }

    else if (strcmp("stats-uniq", argv[1])==0){

        // Cria a estrutura de dados para enviar ao servidor
        InfoPipe info;
        info.num_comandos= 0;
        strcpy(info.client_pipe , client_fifo_path);
    
        for (int i = 2; i < argc; i++) {
            strcpy(info.nome_comando[i-2], argv[i]);
            info.num_comandos++;
        }

        strcpy(info.exec_type, "STATSUNIQ");

        // Envia a informação para o servidor
        enviar_informacao(fd, &info);
        // Lê do pipe e imprime
        read_from_pipe_and_print(client_fifo_path);
    }
    else if (strcmp("stats-command", argv[1])==0){

        // Cria a estrutura de dados para enviar ao servidor
        InfoPipe info;

        info.num_comandos = 0;
        strcpy(info.client_pipe , client_fifo_path);
        for (int i = 3; i < argc; i++) {

            strcpy(info.nome_comando[i-3], argv[i]);
            info.num_comandos++;
        }
        strcpy(info.command, argv[2]);
        strcpy(info.exec_type, "STATSCOMMAND");
        
        // Envia a informação para o servidor
        enviar_informacao(fd, &info);
        // Lê do pipe e imprime
        read_from_pipe_and_print(client_fifo_path);
    }
    else if (strcmp("stats-time", argv[1])==0){

        // Cria a estrutura de dados para enviar ao servidor
        InfoPipe info;
        info.num_comandos = 0;
        strcpy(info.client_pipe , client_fifo_path);
        for (int i = 2; i < argc; i++) {
            strcpy(info.nome_comando[i-2], argv[i]);
            info.num_comandos++;
        }
        strcpy(info.exec_type, "STATSTIME");

        // Envia a informação para o servidor
        enviar_informacao(fd, &info);
        // Lê do pipe e imprime
        read_from_pipe_and_print(client_fifo_path);
    }
    else{
        // Comando inválido
        perror("ERROR: Invalid command");
        exit(EXIT_FAILURE);
    }

    // Fecha os pipes
    close(fd_client);
    close(fd);
    
    // Remove o FIFO do cliente
    if (unlink(client_fifo_path) == -1)
    {
        perror("ERROR: unlink failed");
        exit(EXIT_FAILURE);
    }

    return EXIT_SUCCESS;
}