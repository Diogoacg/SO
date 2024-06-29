# Operating Systems Project - Command Execution Client/Server

## Project Description

This project was developed as part of the Operating Systems course at the University of Minho. The main objective is to implement a client/server system that allows the execution of commands, command pipelines, and queries for process state. The system utilizes named pipes (FIFOs) for communication between the client and the server.

## Project Structure

The project is divided into several parts:

1. **Client**: Responsible for sending commands to the server and processing the responses.
2. **Server**: Responsible for receiving commands from the client, executing the commands, and sending back the process states.
3. **Data Structures**: Definition of structures used to store command and process information.

## Implemented Functionalities

### Client

The client supports various operations:

1. **Command Execution**:
   - `execute -u <command>`: Executes a single command.
   - `execute -p <command1>|<command2>|...|<commandN>`: Executes a pipeline of commands.

2. **State Query**:
   - `status`: Queries the state of running processes.

3. **Statistics**:
   - `stats-uniq`: Shows unique statistics of executed commands.
   - `stats-command <command>`: Shows specific statistics for a command.
   - `stats-time`: Shows time statistics of executed commands.

### Server

The server is responsible for:

1. Receiving and processing commands sent by the client.
2. Executing single commands and command pipelines.
3. Maintaining and sending back the state of running processes to the client.
4. Calculating and sending requested statistics to the client.

## Data Structures

### `Arguments`

Structure used to store arguments passed by the client.

```c
typedef struct {
    char *execution_type;
    char *flag;
    char *commands[MAX_COMMANDS];
    int num_commands;
} Arguments;

```

## InfoPipe
Structure used to store information about processes and commands.

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
# Compilation and Execution
## Compilation
### To compile the project, use the following commands:

```
gcc -o cliente cliente.c
gcc -o servidor servidor.c
```

## Execution
### Start the server:

```
./servidor
```
In another terminal, execute the client with the desired commands:

```
./cliente execute -u "ls -l"
./cliente execute -p "ls -l|grep 'txt'"
./cliente status
./cliente stats-uniq
./cliente stats-command "ls"
./cliente stats-time

```
# Additional Notes

Make sure that the named pipes (../tmp/client_server_fifo and ../tmp/server_client_<pid>) are created correctly and have the necessary permissions.
The project was developed and tested in a Linux environment.

# Conclusion

EThis project demonstrates the application of operating systems concepts such as inter-process communication (IPC) using named pipes, process manipulation (fork and exec), and time management (execution time measurement). The system is a robust tool for executing and monitoring commands in a Linux environment.
