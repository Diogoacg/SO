# Projeto de Sistemas Operativos - Cliente/Servidor de Execução de Comandos

## Descrição do Projeto

Este projeto foi desenvolvido no âmbito da unidade curricular de Sistemas Operativos na Universidade do Minho. O objetivo principal é a implementação de um sistema cliente/servidor que permite a execução de comandos, pipelines de comandos, e consultas de estado dos processos em execução. O sistema utiliza pipes nomeados (FIFOs) para comunicação entre o cliente e o servidor.

## Estrutura do Projeto

O projeto está dividido em várias partes:

1. **Cliente**: Responsável por enviar comandos ao servidor e processar as respostas.
2. **Servidor**: Responsável por receber comandos do cliente, executar os comandos e enviar o estado dos processos de volta ao cliente.
3. **Estruturas de Dados**: Definição das estruturas utilizadas para armazenar as informações dos comandos e processos.

## Funcionalidades Implementadas

### Cliente

O cliente suporta várias operações:

1. **Execução de Comandos**:
   - `execute -u <comando>`: Executa um único comando.
   - `execute -p <comando1>|<comando2>|...|<comandoN>`: Executa uma pipeline de comandos.

2. **Consulta de Estado**:
   - `status`: Consulta o estado dos processos em execução.

3. **Estatísticas**:
   - `stats-uniq`: Mostra estatísticas únicas dos comandos executados.
   - `stats-command <comando>`: Mostra estatísticas específicas para um comando.
   - `stats-time`: Mostra estatísticas de tempo dos comandos executados.

### Servidor

O servidor é responsável por:

1. Receber e processar os comandos enviados pelo cliente.
2. Executar os comandos e pipelines de comandos.
3. Manter e enviar o estado dos processos em execução para o cliente.
4. Calcular e enviar estatísticas solicitadas pelo cliente.

## Estruturas de Dados

### `Argumentos`

Estrutura utilizada para armazenar os argumentos passados pelo cliente.

```c
typedef struct {
    char *tipo_execucao;
    char *flag;
    char *comandos[MAX_COMANDOS];
    int num_comandos;
} Argumentos;
```

## InfoPipe
Estrutura utilizada para armazenar informações sobre os processos e comandos.

```c
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

```
#Compilação e Execução
##Compilação
###Para compilar o projeto, utilize o comando:

```
gcc -o cliente cliente.c
gcc -o servidor servidor.c
```

## Execução
### Inicie o servidor:

```
./servidor
```
Em outro terminal, execute o cliente com os comandos desejados:

```
./cliente execute -u "ls -l"
./cliente execute -p "ls -l|grep 'txt'"
./cliente status
./cliente stats-uniq
./cliente stats-command "ls"
./cliente stats-time

```
# Notas Adicionais

Certifique-se de que os pipes nomeados (../tmp/client_server_fifo e ../tmp/server_client_<pid>) são criados corretamente e têm as permissões necessárias.
O projeto foi desenvolvido e testado em um ambiente Linux.

#Conclusão

Este projeto demonstra a aplicação de conceitos de sistemas operativos, tais como comunicação entre processos (IPC) usando pipes nomeados, manipulação de processos (fork e exec), e gestão de tempo (medição de tempo de execução). O sistema é uma ferramenta robusta para a execução e monitorização de comandos em um ambiente Unix/Linux.
