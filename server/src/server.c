#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include<time.h>
#include<string.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<sys/wait.h>
#include<signal.h>
#include<fcntl.h>

#include"include/matinverse.h"
#include"include/kmeans.h"

#define RED "\033[1;31m"
#define GREEN "\033[1;32m"
#define BLUE "\033[1;34m"
#define CYAN "\033[1;36m"
#define MAGENTA "\033[1;35m"
#define NORMAL "\033[0m"

#define ERROR(err_str) fprintf(stderr, RED"\nERROR: %s\n"NORMAL, err_str);
#define WARNING(warn_str) fprintf(stderr, MAGENTA"\nERROR: %s\n"NORMAL, warn_str);

#define PORT 8080
#define BUFFER_SIZE 8192 // bytes
#define MAX_COMMAND_LENGTH 512
#define INTER_DIR "intermediate_files/"

// default server settings
int server_port = -1;
int daemon_enabled = 0;
char *strategy_in_use;

// to create and append data to log files even after restarting
FILE* ensureLogfileExists(char *filename){
    FILE* fp = fopen(filename, "r");
    if(fp==NULL){
        fp = fopen(filename, "w+");
        return fp;
    }
    fclose(fp);
    fp = fopen(filename, "a");
    return fp;
}

// returns true if the file doesn't exist
int checkFileExists(char *filename, size_t *filesize){
    FILE *fp = fopen(filename, "r");
    if(fp==NULL){
        return 1;
    }
    fseek(fp, 0L, SEEK_END);
    *filesize = ftell(fp);
    fclose(fp);
    return 0;
}

// used to send the result file to client
void sendFile(char *filename, int sockettt){
    FILE *fp = fopen(filename, "r");
    if(fp == NULL){
        ERROR("Coudn't read file!")
        return;
    }
    size_t file_size;
    checkFileExists(filename, &file_size);
    char *buffer = (char *)malloc(file_size*sizeof(char));
    size_t bytes_read = fread(buffer, sizeof(char), file_size, fp);
    printf("Sending %ld bytes...\n", file_size);
    send(sockettt, buffer, bytes_read, 0);
    fclose(fp);
    free(buffer);
    return;
}

// used to receive file from client
void receiveFile(char *filename, int sockettt, size_t file_size){
    FILE *fp = fopen(filename, "w+");
    // read the data coming from the client
    char *buffer = (char *)malloc(file_size*sizeof(char));
    printf("Receiving %ld bytes...\n", file_size);
    ssize_t total_received_bytes = recv(sockettt, buffer, file_size, 0);
    if(total_received_bytes < file_size){
        total_received_bytes += recv(sockettt, buffer+total_received_bytes, file_size, 0);
    }
    printf("Received %ld bytes.\n", total_received_bytes);
    fwrite(buffer, sizeof(char), total_received_bytes, fp);
    fclose(fp);
    free(buffer);
    return;
}

// cannot exit program when server is running, so document all detected problems into logfile
void log_runtime_err(char* error_message){
    time_t currentDateTime = time(NULL);
    struct tm *tm = localtime(&currentDateTime);
    char currentDate[18];
    sprintf(currentDate, "logs/%d%02d%02d.log", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday);
    FILE* fp = ensureLogfileExists(currentDate);
    fprintf(fp, "@%02d:%02d:%02d --> %s", tm->tm_hour, tm->tm_min, tm->tm_sec, error_message);
    fclose(fp);
    return;
}

// handler that gets triggered when a child process exits -- prevents zombie process creation
void sigchld_handler(int signo) {
    int reaped_procs = 0;
    // Reap all terminated child processes
    while (waitpid(-1, NULL, WNOHANG) > 0) {
        reaped_procs++;
    }
    char log_msg[64];
    sprintf(log_msg, "Process(s) terminated: %d\n", reaped_procs);
    log_runtime_err(log_msg);
    return;
}

// process the client's request appropriately
int process_request(int client_socket, char *request_message, int request_count){
    // define the variables needed
    char err_response[MAX_COMMAND_LENGTH] = "ERROR: Invalid request!";
    char main_command[10];
    int matrix_size = -1;
    int num_clusters = -1;
    int max_randnum = -1;
    char option_I[10];
    int option_P = -1;
    size_t file_size = 0;
    
    // read the client request to figure out the parameters
    sscanf(request_message, "command: %s matsize: %d I: %s P: %d max_randnum: %d num_clusters: %d filesize: %ld\n", main_command, &matrix_size, option_I, &option_P, &max_randnum, &num_clusters, &file_size);

    int compute_inverse = (strcmp(main_command, "matinvpar")==0);
    int compute_kmeans = (strcmp(main_command, "kmeanspar")==0);
    
    // confirm that the received command is valid
    if(!(compute_inverse || compute_kmeans)){
        log_runtime_err("Malformed request encountered!\n");
        if(send(client_socket, err_response, strlen(err_response), 0)<0){
            log_runtime_err("Coudn't send error response!\n");
        }
        close(client_socket);
        exit(EXIT_FAILURE);
    }
    printf("Received command: %s\n", main_command);
    
    // for k-means, get the datafile too
    // create a unique file to hold the data
    char kmeans_file[64];
    if(compute_kmeans){
        sprintf(kmeans_file, INTER_DIR"client_%d.txt", getpid());
        receiveFile(kmeans_file, client_socket, file_size);
    }
    
    char client_resfile[MAX_COMMAND_LENGTH];

    if(compute_inverse){
        sprintf(client_resfile, INTER_DIR"client_%d_matinv_result_%d.txt", getpid(), request_count);
        compute_matinv(matrix_size, max_randnum, option_I, client_resfile);
    } else{
        sprintf(client_resfile, INTER_DIR"client_%d_kmeans_result_%d.txt", getpid(), request_count);
        run_kmeans(-1, num_clusters, kmeans_file, client_resfile);
    }

    // send the calculated results via file transfer
    char response[MAX_COMMAND_LENGTH];
    size_t output_filesize = 0;
    checkFileExists(client_resfile, &output_filesize);
    sprintf(response, "request_count: %d output_filesize: %ld", request_count, output_filesize);
    send(client_socket, response, MAX_COMMAND_LENGTH, 0);
    sendFile(client_resfile, client_socket);

    printf("Solution dispatched to client.\n");
    
    return EXIT_SUCCESS;
}

// handles any newly connected client in a new process
void process_new_connection(int client_socket){
    // a client can make multiple math-requests
    int request_count = 1;
    while(1){
        // read the client request in a predefined buffer
        char buffer[MAX_COMMAND_LENGTH];
        memset(buffer, 0, MAX_COMMAND_LENGTH);

        // read the incoming request
        ssize_t bytes_received = recv(client_socket, buffer, MAX_COMMAND_LENGTH, 0);
        if (bytes_received < 0) {
            log_runtime_err("Couldn't receive data from socket!\n");
            close(client_socket);
            return;
        }
        
        // check for the shutdown command, or if the peer (client) has closed the connection
        if(strcmp(buffer, "shutdown")==0 || bytes_received==0){
            break;
        }
        
        // log the request for future references
        log_runtime_err(buffer);
        
        // process the client's request appropriately
        request_count += !process_request(client_socket, buffer, request_count);
    }
    // close the connection
    close(client_socket);
    printf("Closed connection with client.\n");
}

// function will be used to print the help page in the parser
void print_help() {
    printf("Usage: server -h -p port -d -s strategy\n");
    printf("Options:\n");
    printf("  -h               Print this help message\n");
    printf("  -p port          Listen to port number port\n");
    printf("  -d               Run as a daemon[NOT IMPLEMENTED!]\n");
    printf("  -s strategy      Specify the request handling strategy (fork, muxbasic, muxscale)[ONLY FORK STRATEGY!]\n");
}

// parse the command line parameters
void parse_options(int argc, char** argv){
    char* prog;
    prog = *argv;
    while (++argv, --argc > 0)
        if (**argv == '-')
            switch (*++ * argv) {
            case 'h':
                print_help();
                exit(EXIT_SUCCESS);
                break;
            case 's':
                --argc;
                strategy_in_use = *++argv;
                exit(3);
                break;
            case 'p':
                --argc;
                server_port = atoi(*++argv);
                break;
            case 'd':
                --argc;
                daemon_enabled = 1;
                break;
            default:
                ERROR("Invalid option used!")
                printf("HELP: try %s -h \n\n", prog);
                exit(EXIT_SUCCESS);
                break;
            }
}






int main(int argc, char **argv){
    // parse the command-line parameters and adjust server settings
    parse_options(argc, argv);
    if(daemon_enabled){
        pid_t main_pid = fork();
        if(main_pid < 0){
            perror("Error: Unable to daemonize server!");
        }
        else if(main_pid > 0){
            printf(MAGENTA"Server has been successfully daemonized! [Process ID: %d]\n"NORMAL, main_pid);
            return EXIT_SUCCESS;
        }
        else{
            if(setsid() < 0){
                ERROR("Couldn't create a new independent session!")
                exit(EXIT_FAILURE);
            }
            // close file descriptors for I/O
            close(STDIN_FILENO);
            close(STDOUT_FILENO);
            close(STDERR_FILENO);
        }
    }


    server_port = (server_port == -1) ? PORT : server_port;

    // socket initialization
    int server_socket, client_socket;
    struct sockaddr_in server_address, client_address;
    socklen_t address_len = sizeof(client_address);

    // creating the socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Unable to create a socket!");
        return EXIT_FAILURE;
    }

    // setting up the server address struct
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(server_port);

    // set socket options to reuse the port after restart
    int opt = 1;
    if(setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0){
        perror("Unable to set socket options!");
        return EXIT_FAILURE;
    }

    // binding socket to specified IP and port
    if(bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0){
        perror("Unable to bind socket!");
        return EXIT_FAILURE;
    }

    // now listen for any incoming requests
    if(listen(server_socket, 10) < 0){
        perror("Unable to listen for requests!");
        return EXIT_FAILURE;
    }

    printf("Listening for clients at port:%d...\n", server_port);

    if(signal(SIGCHLD, sigchld_handler) == SIG_ERR){
        perror("Unable to setup the signal handler for dead-child reaper!");
        return EXIT_FAILURE;
    }

    while(1){
        // accept a client's connection request
        client_socket = accept(server_socket, (struct sockaddr *)&client_address, &address_len);
        if(client_socket < 0){
            log_runtime_err("Couldn't accept a client's connection request!\n");
            continue;
        }
        // create a new process and interact with the client there
        pid_t pid_req_processor = fork();
        if(pid_req_processor < 0){
            log_runtime_err("CRITICAL ERROR: Unable to create new process to handle client's request!\n\n");
        } else if(pid_req_processor == 0){
            close(server_socket);
            printf("Connected to a new client.\n");
            // handle the client request in a separate function
            process_new_connection(client_socket);
            return EXIT_SUCCESS;
        } else {
            // close the client socket in the parent process
            close(client_socket);
        }
    }

    // close the server socket when everything is done
    close(server_socket);

    return EXIT_SUCCESS;
}
