#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>

#define SERVER_PIPE "client_server_fifo"
#define CLIENT_PIPE "server_client_fifo"
#define MAX_RUNNING_PIDS 100

typedef struct
{
    int pid;
    char nome_comando[10][1024];
    int num_comandos;
    long timestamp;
    char status[5];
    char exec_type[10];
    long exec_elapsed_time;
} InfoPipe;

typedef struct{
    int pid;
    int num_comandos;
    char nome_comando[10][1024];
    long timestamp;
}RunningPIDS;

//lista de RunningPIDS
RunningPIDS running_pids[MAX_RUNNING_PIDS];

int num_running_pids = 0;


int log_file_already_exist(const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (file != NULL)
    {
        fclose(file);
        return 1;
    }
    return 0;
}




void write_to_log_file(const InfoPipe *info)
{
    char log_filename[64];
    sprintf(log_filename, "logs/log_pid_%d.txt", info->pid);

    if (log_file_already_exist(log_filename) == 0)
    {
        FILE *log_file = fopen(log_filename, "a");
        if (!log_file)
        {
            perror("Erro ao abrir o arquivo de log");
            exit(EXIT_FAILURE);
        }


        fprintf(log_file, "PID: %d\nElepsedTime: %ld\nStatus: %s\n", info->pid, info->exec_elapsed_time, info->status);
        for (int i = 0; i < info->num_comandos; i++)
        {
            fprintf(log_file, "Command %d: %s\n", i + 1, info->nome_comando[i]);
        }
        fprintf(log_file, "\n");

        fclose(log_file);
    }
}

void add_procecc_to_running_list(int pid, char names[10][1024], long start_time,int num_comandos)
{
    running_pids[num_running_pids].pid = pid;
    running_pids[num_running_pids].timestamp = start_time;
    running_pids[num_running_pids].num_comandos = num_comandos;
    for(int i = 0; i < num_comandos; i++) {
        strcpy(running_pids[num_running_pids].nome_comando[i], names[i]);
    }
    num_running_pids++;
}

void remove_pid_from_running_list(int pid)
{
    for (int i = 0; i < num_running_pids; ++i)
    {
        if (running_pids[i].pid == pid)
        {
            for (int j = i; j < num_running_pids - 1; ++j)
            {
                running_pids[j] = running_pids[j + 1];
            }
            num_running_pids--;
            break;
        }
    }
}

void print_running_pids()
{
    printf("PIDs em execução:\n");
    for (int i = 0; i < num_running_pids; ++i)
    {
        printf("%d\n", running_pids[i].pid);
    }
}

void send_pid_info_to_client(int fd) {
    for (int i = 0; i < num_running_pids; ++i) {
        InfoPipe info;
        strcpy(info.exec_type, "STATUS");
        info.pid = running_pids[i].pid;
        info.num_comandos = running_pids[i].num_comandos;
        struct timeval current_time;
        gettimeofday(&current_time, NULL);
        info.exec_elapsed_time = (current_time.tv_sec * 1000 + current_time.tv_usec / 1000) - running_pids[i].timestamp;
        for(int j=0; j < running_pids[i].num_comandos; j++){
            strcpy(info.nome_comando[j], running_pids[i].nome_comando[j]);
            printf("%s\n", info.nome_comando[j]); 
            printf("%d\n", running_pids[i].num_comandos);
        }
        write(fd, &info, sizeof(InfoPipe));
    }
}

int main()
{
    int fd, fd_client;
    InfoPipe info;

    if (mkfifo(SERVER_PIPE, 0666) < 0 && errno != EEXIST)
    {
        perror("Erro ao criar pipe");
        exit(EXIT_FAILURE);
    }

    if ((fd = open(SERVER_PIPE, O_RDONLY | O_NONBLOCK)) < 0)
    {
        perror("Erro ao abrir o pipe");
        exit(EXIT_FAILURE);
    }

    if ((fd_client = open(CLIENT_PIPE, O_WRONLY | O_NONBLOCK)) < 0)
    {
        perror("Erro ao abrir o pipe");
        exit(EXIT_FAILURE);
    }

    fd_set read_fds;
    struct timeval timeout;
    int select_result;

    while (1)
    {
        FD_ZERO(&read_fds);
        FD_SET(fd, &read_fds);

        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        select_result = select(fd + 1, &read_fds, NULL, NULL, &timeout);

         if (select_result > 0 && FD_ISSET(fd, &read_fds))
        {
            if (read(fd, &info, sizeof(InfoPipe)) < 0)
            {
                perror("Erro ao ler do pipe");
                exit(EXIT_FAILURE);
            }

            if (strcmp(info.exec_type, "EXECUTE") == 0)
            {
                if (strcmp(info.status, "EXEC") == 0)
                {   
                    struct timeval current_time;
                    gettimeofday(&current_time, NULL);
                    add_procecc_to_running_list(info.pid, info.nome_comando, current_time.tv_sec * 1000 + current_time.tv_usec / 1000, info.num_comandos);
                }
                else if (strcmp(info.status, "END") == 0)
                {
                    remove_pid_from_running_list(info.pid);
                    write_to_log_file(&info);
                }
            }
            else if (strcmp(info.exec_type, "STATUS") == 0) {
                send_pid_info_to_client(fd_client);
    }
        }
    }

    close(fd);

    return EXIT_SUCCESS;
}
