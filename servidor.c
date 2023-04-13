#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#define SERVER_PIPE "client_server_fifo"
#define CLIENT_PIPE "server_client_fifo"


int main() {
    int server_fd, client_fd;
    char buf[256];
    int bytes_read;
    mkfifo(SERVER_PIPE, 0666);
    // Abre o pipe nomeado do servidor
    server_fd = open(SERVER_PIPE, O_RDONLY);
    
    if (server_fd == -1) {
        perror("Erro ao abrir o pipe do servidor");
        exit(EXIT_FAILURE);
    }

    // Lê o comando enviado pelo cliente
    while(1) {
        //printf("Aguardando comando do cliente...\n");
        if ((bytes_read = read(server_fd, buf, 256)) > 0) {
            buf[bytes_read] = '\0'; // Adiciona o caractere nulo no final da string
            printf("Comando recebido do cliente: %s\n", buf);
            }
                
                
                
        }

    // Fecha o pipe do servidor
    close(server_fd);
    unlink(SERVER_PIPE);

    return 0;
}