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
#include <signal.h>

#define SERVER_PIPE "../tmp/client_server_fifo"
#define CLIENT_PIPE_PREFIX "../tmp/server_client_"
#define MAX_BUF 1024
#define INFO_PIPE_SIZE sizeof(InfoPipe)
#define MAX_RUNNING_PIDS 100
#define BUFFER_SIZE 256

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

typedef struct{
    int pid;
    int num_comandos;
    char nome_comando[10][1024];
    long timestamp;
}RunningPIDS;

// Guarda os PIDs dos processos que estão sendo executados
RunningPIDS running_pids[MAX_RUNNING_PIDS];
int num_running_pids = 0;


// Verifica se o arquivo de log já existe
int log_file_already_exist(const char *filename) {
    int fd = open(filename, O_RDONLY);
    if (fd != -1) {
        close(fd);
        return 1;
    }
    return 0;
}

// Escreve as informações do processo no arquivo de log
void write_to_log_file(const InfoPipe *info, const char *log_location)
{
    // Cria o nome do arquivo de log com base no local e no PID
    char log_filename[64];
    sprintf(log_filename, "%s/log_pid_%d.txt", log_location, info->pid);

    // Verifica se o arquivo de log já existe, caso contrário, cria um novo arquivo
    if (log_file_already_exist(log_filename) == 0)
    {
        // Abre o arquivo de log para anexar informações
        int log_file = open(log_filename, O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (log_file < 0)
        {
            perror("Erro ao abrir o arquivo de log");
            exit(EXIT_FAILURE);
        }

        // Prepara as informações do processo para serem escritas no arquivo de log
        char buffer[1024];
        int length = snprintf(buffer, sizeof(buffer), "PID: %d\nElapsedTime: %ld\nStatus: %s\n", info->pid, info->exec_elapsed_time, info->status);
        for (int i = 0; i < info->num_comandos; i++)
        {
            length += snprintf(buffer + length, sizeof(buffer) - length, "Command %d: %s\n", i + 1, info->nome_comando[i]);
        }
        snprintf(buffer + length, sizeof(buffer) - length, "\n");

        // Escreve as informações do processo no arquivo de log
        write(log_file, buffer, strlen(buffer));

        // Fecha o arquivo de log
        close(log_file);
    }
}
// Adiciona um processo à lista de processos em execução
void add_procecc_to_running_list(int pid, char names[10][1024], long start_time, int num_comandos)
{
    running_pids[num_running_pids].pid = pid;
    running_pids[num_running_pids].timestamp = start_time;
    running_pids[num_running_pids].num_comandos = num_comandos;
    // Copia os nomes dos comandos para a posição atual da lista
    for (int i = 0; i < num_comandos; i++)
    {
        strcpy(running_pids[num_running_pids].nome_comando[i], names[i]);
    }
    // Incrementa o número de processos em execução
    num_running_pids++;
}

// Remove um processo da lista de processos em execução pelo PID
void remove_pid_from_running_list(int pid)
{
    // Itera pelos processos em execução na lista
    for (int i = 0; i < num_running_pids; ++i)
    {
        // Verifica se o PID do processo atual corresponde ao PID desejado
        if (running_pids[i].pid == pid)
        {
            // Desloca todos os processos à direita do processo encontrado para a esquerda
            for (int j = i; j < num_running_pids - 1; ++j)
            {
                running_pids[j] = running_pids[j + 1];
            }

            // Decrementa o número de processos em execução
            num_running_pids--;

            // Interrompe o loop, pois o processo foi removido
            break;
        }
    }
}


// Envia informações dos PIDs para o cliente através de um pipe nomeado
void send_pid_info_to_client(char* client_pipe) {
    // Iterar pelos processos em execução
    for (int i = 0; i < num_running_pids; ++i) {
        InfoPipe info;

        // Inicializar e abrir o pipe do cliente
        int client_fd;
        client_pipe[strcspn(client_pipe,"\n\r")] = 0;
        client_fd = open(client_pipe, O_WRONLY);
        
        // Verificar se houve erro ao abrir o FIFO do cliente
        if (client_fd < 0) {
            perror("ERROR: was not possible to open the client pipe");
            return;
        }

        // Preencher as informações do processo atual
        strcpy(info.exec_type, "END");
        info.pid = running_pids[i].pid;
        info.num_comandos = running_pids[i].num_comandos;
        
        // Calcular e preencher o tempo de execução
        struct timeval current_time;
        gettimeofday(&current_time, NULL);
        info.exec_elapsed_time = (current_time.tv_sec * 1000 + current_time.tv_usec / 1000) - running_pids[i].timestamp;
        
        // Copiar nomes dos comandos
        for(int j=0; j < running_pids[i].num_comandos; j++){
            strcpy(info.nome_comando[j], running_pids[i].nome_comando[j]);
        }
        
        // Escrever informações no pipe do cliente
        write(client_fd, &info, INFO_PIPE_SIZE);
    }
}

int send_string_to_client(const char *client_pipe, const char *string) {
    int client_fd;

    // Abrindo o pipe nomeado (FIFO) para escrita
    client_fd = open(client_pipe, O_WRONLY);
    if (client_fd < 0) {
        perror("ERROR: open client pipe failed");
        return -1;
    }

    // Copiando a string e adicionando um caractere nulo no final
    size_t str_length = strlen(string);
    char str[str_length + 1];
    strcpy(str, string);
    str[str_length] = '\0';

    // Enviando a string para o cliente através do pipe nomeado (FIFO)
    ssize_t bytes_written = write(client_fd, str, str_length + 1);
    if (bytes_written != str_length + 1) {
        perror("ERROR: write to client pipe failed");
        close(client_fd);
        return -1;
    }

    // Fechando o descritor de arquivo do pipe nomeado (FIFO)
    close(client_fd);

    return 0;
}

// Retorna 1 se o PID for encontrado na lista de processos em execução, 0 caso contrário
int find_pid(const char* pid, InfoPipe info, int n) {
    for (int i = 0; i < n; i++) {
        if (strcmp(info.nome_comando[i], pid) == 0)
            return 1;
    }
    return 0;
}

// Calcula e envia o tempo de execução total dos processos especificados para o cliente
void stats_time(InfoPipe info, int n, const char *logdir_path, char *client_fd) {
    // Abrir o diretório onde estão localizados os arquivos de log
    DIR *d;
    struct dirent *dir;
    d = opendir(logdir_path);
    int elapsedTime = 0;

    // Verificar se o diretório foi aberto com sucesso
    if (d) {
        // Iterar por todos os arquivos do diretório
        while ((dir = readdir(d)) != NULL) {
            // Procurar por arquivos de log que iniciem com "log_pid_"
            if (strstr(dir->d_name, "log_pid_") != NULL) {
                // Extrair o PID do nome do arquivo
                char pid[BUFFER_SIZE];
                sscanf(dir->d_name, "log_pid_%[^.].txt", pid);

                // Verificar se o PID está na lista de PIDs enviados pelo cliente
                if (find_pid(pid, info, n)) {
                    // Abrir o arquivo de log e ler o tempo de execução
                    char filename[BUFFER_SIZE];
                    snprintf(filename, sizeof(filename)+1, "%s/%s", logdir_path, dir->d_name);
                    int file = open(filename, O_RDONLY);
                    
                    // Verificar se houve erro ao abrir o arquivo de log
                    if (file < 0) {
                        perror("ERROR: Failed to open log file");
                        return;
                    }
                    
                    // Ler o arquivo de log e somar os tempos de execução
                    if (file >= 0) {
                        int tempTime;
                        char buffer[BUFFER_SIZE];
                        ssize_t bytes_read;
                        while ((bytes_read = read(file, buffer, BUFFER_SIZE - 1)) > 0) {
                            buffer[bytes_read] = '\0';
                            const char *line = buffer;
                            
                            // Encontrar e somar os tempos de execução no arquivo de log
                            while ((line = strstr(line, "ElapsedTime:")) != NULL) {
                                sscanf(line, "ElapsedTime: %d", &tempTime);
                                elapsedTime += tempTime;
                                line += strlen("ElapsedTime:");
                            }
                        }
                        close(file);
                    }
                }
            }
        }
        closedir(d);
    }

    // Enviar o tempo total de execução para o cliente
    char string[BUFFER_SIZE];
    snprintf(string, sizeof(string), "Total execution time is %d ms\n", elapsedTime);
    send_string_to_client(client_fd, string);
}


// Calcula e envia a quantidade de vezes que um comando foi executado para o cliente
void stats_command(const char *command, InfoPipe info, int pid_count, const char *logdir_path, char* client_fd) {
    // Abrir o diretório onde estão localizados os arquivos de log
    DIR *d;
    struct dirent *dir;
    d = opendir(logdir_path);
    
    if (d == NULL) {
        perror("ERROR: open log directory");
        return;
    }

    char filename[256];
    int ex_count = 0;

    // Iterar por todos os arquivos no diretório
    while ((dir = readdir(d)) != NULL) {
        for (int i = 0; i < pid_count; i++) {
            // Procurar arquivos de log associados aos PIDs enviados pelo cliente
            if (strstr(dir->d_name, info.nome_comando[i]) != NULL) {
                snprintf(filename, sizeof(filename)+1, "%s/%s", logdir_path , dir->d_name);

                // Abrir o arquivo de log
                int fd = open(filename, O_RDONLY);
                if (fd == -1) {
                    perror("ERROR: open log file");
                    return;
                }

                char line[256];
                ssize_t bytesRead;
                int index = 0;
                char ch;

                // Ler o arquivo linha por linha
                while ((bytesRead = read(fd, &ch, 1)) > 0) {
                    if (ch == '\n') {
                        line[index] = '\0';
                        
                        // Verificar se a linha contém o comando especificado
                        if (strstr(line, command) != NULL) {
                            ex_count++;
                        }
                        index = 0;
                    } else {
                        line[index++] = ch;
                        if (index == sizeof(line) - 1) {
                            line[index] = '\0';
                            index = 0;
                        }
                    }
                }

                close(fd);
                break;
            }
        }
    }

    closedir(d);

    // Enviar a quantidade de vezes que o comando foi executado para o cliente
    char string[256];
    snprintf(string, sizeof(string), "%s was executed %d times\n", command, ex_count);
    send_string_to_client(client_fd, string);
}

// Verifica se uma linha de um arquivo de log é um comando
bool is_command_line(const char *line) {
    int nr;
    return sscanf(line, "Command %d: ", &nr) == 1;
}

// Encontra e envia todos os comandos únicos executados nos arquivos de log para o cliente
void stats_uniq(InfoPipe info, int pid_count, const char *logdir_path, char* client_fd) {
    DIR *d;
    struct dirent *dir;

    // Abrir o diretório onde estão localizados os arquivos de log
    d = opendir(logdir_path);
    if (d == NULL) {
        perror("ERROR: Failed to open log directory");
        return;
    }

    const int MAX_COMMANDS = 1024;
    char unique_commands[MAX_COMMANDS][256];
    int unique_count = 0;

    char filename[256];
    memset(unique_commands, 0, sizeof(unique_commands));

    // Iterar por todos os arquivos no diretório
    while ((dir = readdir(d)) != NULL) {
        for (int i = 0; i < pid_count; i++) {
            // Procurar arquivos de log associados aos PIDs enviados pelo cliente
            if (strstr(dir->d_name, info.nome_comando[i]) != NULL) {
                snprintf(filename, sizeof(filename)+1, "%s/%s", logdir_path , dir->d_name);

                // Abrir o arquivo de log
                int fd = open(filename, O_RDONLY);
                if (fd == -1) {
                    perror("ERROR: Failed to open log file");
                    return;
                }

                char line[256];
                ssize_t bytesRead;
                int index = 0;
                char ch;

                // Ler o arquivo linha por linha
                while ((bytesRead = read(fd, &ch, 1)) > 0) {
                    if (ch == '\n') {
                        line[index] = '\0';

                        // Verificar se a linha é uma linha de comando
                        if (is_command_line(line)) {
                            const char *command_start = strchr(line, ':') + 2;

                            bool command_found = false;
                            // Verificar se o comando já foi adicionado à lista de comandos únicos
                            for (int j = 0; j < unique_count; j++) {
                                if (strcmp(command_start, unique_commands[j]) == 0) {
                                    command_found = true;
                                    break;
                                }
                            }

                            // Adicionar comando à lista de comandos únicos se não estiver presente
                            if (!command_found && unique_count < MAX_COMMANDS) {
                                strcpy(unique_commands[unique_count++], command_start);
                            }
                        }

                        index = 0;
                    } else {
                        line[index++] = ch;
                        if (index == sizeof(line) - 1) {
                            line[index] = '\0';
                            index = 0;
                        }
                    }
                }

                close(fd);
                break;
            }
        }
    }

    closedir(d);

    // Enviar a lista de comandos únicos para o cliente
    char string[256] = "";
    for (int i = 0; i < unique_count; i++) {
        strcat(string, unique_commands[i]);
        strcat(string, "\n");
    }
    send_string_to_client(client_fd, string);
}

//remove o FIFO do servidor
void sigint_handler(int signum) {
    if (unlink(SERVER_PIPE) == -1) {
        perror("ERROR: unlink");
        exit(1);
    }
    exit(0);
}

// Função principal do servidor que recebe os pedidos do cliente e envia as respostas adequadas
int main(int argc, char* argv[]) {
    int fd;
    InfoPipe info;

    // Cria o FIFO do servidor se ele não existir
    if(log_file_already_exist(SERVER_PIPE) == 0) {
        if (mkfifo(SERVER_PIPE, 0777) == -1) {
            perror("ERROR: mkfifo server pipe");
            return (EXIT_FAILURE);
        }
    }

    //Cria o diretório de logs se ele não existir
    if(log_file_already_exist(argv[1]) == 0) {
        if (mkdir(argv[1], 0777) == -1) {
            perror("ERROR: mkdir logdir");
            return (EXIT_FAILURE);
        }
    }
    // Associa o sinal SIGINT ao handler sigint_handler
    signal(SIGINT, sigint_handler);

    // Abre o FIFO do servidor para leitura
    fd = open(SERVER_PIPE, O_RDONLY);
    if (fd < 0) {
        perror("ERROR: open server pipe");
        return (EXIT_FAILURE);
    }
    
    // Enquanto o servidor estiver em funcionamento
    while (1) {
        // Lê o pedido do cliente
        while (read(fd, &info, INFO_PIPE_SIZE) > 0) {
            // Verifica se o pedido é para executar um comando
            if (strcmp(info.exec_type, "EXECUTE") == 0){
                // Verifica se o comando já terminou ou ainda está sendo executado
                if (strcmp(info.status, "EXEC") == 0){
                    struct timeval current_time;
                    gettimeofday(&current_time, NULL);
                    add_procecc_to_running_list(info.pid, info.nome_comando, current_time.tv_sec * 1000 + current_time.tv_usec / 1000, info.num_comandos);
                } else if (strcmp(info.status, "END") == 0) {
                    write_to_log_file(&info, argv[1]);
                    remove_pid_from_running_list(info.pid);
                }
            } else if (strcmp(info.exec_type, "STATSUNIQ") == 0) { // Pedido de estatísticas únicas
                stats_uniq(info, info.num_comandos, argv[1], info.client_pipe);
            } else if (strcmp(info.exec_type, "STATSCOMMAND") == 0) { // Pedido de estatísticas por comando
                stats_command(info.command, info,info.num_comandos, argv[1], info.client_pipe);
            } else if (strcmp(info.exec_type, "STATSTIME") == 0) { // Pedido de estatísticas por tempo
                stats_time(info, info.num_comandos, argv[1], info.client_pipe);
            } else if(strcmp(info.exec_type, "STATUS") == 0) { // Pedido de status do processo
                send_pid_info_to_client(info.client_pipe);
            } else {
                perror("ERROR: invalid exec_type");
            }
        }
    }       
    close(fd);

    return EXIT_SUCCESS;
}

