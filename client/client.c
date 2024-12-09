#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<readline/readline.h>
#include<readline/history.h>

#define GREEN "\033[1;32m"
#define BLUE "\033[1;34m"
#define NORMAL "\033[0m"
#define RED "\033[1;31m"
#define CYAN "\033[1;36m"
#define MAGENTA "\033[1;35m"

#define ERROR(err_str) fprintf(stderr, RED"\nERROR: %s\n"NORMAL, err_str);
#define WARNING(warn_str) fprintf(stderr, MAGENTA"\nERROR: %s\n"NORMAL, warn_str);

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080
#define MAX_COMMAND_LENGTH 512
#define BUFFER_SIZE 8192

#define RESULTS_DIR "results/"

// client settings for connecting to the server
char *server_ip = SERVER_IP;
int connection_port = SERVER_PORT;

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

// sends a file to the server, needed for k-means results
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

// receives the result file from server
void receiveFile(char *filename, int sockettt, size_t file_size){
    FILE *fp = fopen(filename, "w+");
    // read the data coming from the client
    char *buffer = (char *)malloc(file_size*sizeof(char));
    printf("Receiving %ld bytes...\n", file_size);
    ssize_t total_received_bytes = recv(sockettt, buffer, file_size, 0);
    while(total_received_bytes < file_size){
        total_received_bytes += recv(sockettt, buffer+total_received_bytes, file_size, 0);
    }
    printf("Received %ld bytes.\n", total_received_bytes);
    fwrite(buffer, sizeof(char), total_received_bytes, fp);
    fclose(fp);
    free(buffer);
    return;
}

// function will be used to print the help page in the parser
void print_help() {
    printf("Usage: client -h -ip server_ip -p server_port\n");
    printf("Options:\n");
    printf("  -h                  Print this help message\n");
    printf("  -ip server_ip       Listen to port number port\n");
    printf("  -p server_port      Listen to port number port\n");
}

// parse the command line options for this program
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
            case 'i':
                --argc;
                char det = *++ *argv;
                if(det == 'p')
                    server_ip = *++argv;
                else{
                    ERROR("Invalid option used!")
                    printf("HELP: try %s -h\n\n", prog);
                    exit(EXIT_SUCCESS);
                }
                break;
            case 'p':
                --argc;
                connection_port = atoi(*++argv);
                break;
            default:
                ERROR("Invalid option used!")
                printf("HELP: try %s -h \n\n", prog);
                exit(EXIT_SUCCESS);
                break;
            }
}

// to print help regarding commands in the client-shell
void print_command_help(int command_type){
    if(command_type){
        printf("\nUsage: matinvpar [-n problemsize]\n");
        printf("           [-D] show default values \n");
        printf("           [-h] help \n");
        printf("           [-I init_type] fast/rand \n");
        printf("           [-m maxnum] max random no \n");
        printf("           [-P print_switch] 0/1 \n");
    } else{
        printf("\nUsage: kmeanspar [-f filepath]\n");
        printf("           [-D] show default values \n");
        printf("           [-h] help \n");
        printf("           [-k] cluster_size \n");
    }
}

// will parse the client-shell commands using this
void parse_commands(char *command, int server_socket){
    // initialize the input variables
    char main_command[10];
    int matrix_size = 5;
    int max_randnum = 15;
    char option_I[10] = "fast";
    int option_P = 1;
    int num_clusters = 2;
    char file_path[100] = "";
    size_t file_size = 0;

    // get the main command first
    if(sscanf(command, "%s", main_command) <= 0){
        return;
    }
    
    // check for command's validity
    int compute_inverse = (strcmp(main_command, "matinvpar")==0);
    int compute_kmeans = (strcmp(main_command, "kmeanspar")==0);
    int shutdown = (strcmp(main_command, "shutdown")==0);
    if(shutdown){
        send(server_socket, main_command, 10, 0);
        exit(EXIT_SUCCESS);
    }
    // don't proceed if anything important is invalid
    if(!(compute_inverse || compute_kmeans || shutdown)){
        ERROR("Invalid command used!")
        printf("Try using `matinvpar` or `kmeanspar`. Use -h flag to get help\n");
        return;
    }

    // now parse the options
    char *token = strtok(command, " ");
    while(token != NULL){
        if (strcmp(token, "-n") == 0) {
            token = strtok(NULL, " ");
            if (token != NULL) {
                matrix_size = atoi(token);
            }
        } else if (strcmp(token, "-I") == 0) {
            token = strtok(NULL, " ");
            if (token != NULL) {
                strncpy(option_I, token, sizeof(option_I) - 1);
                option_I[sizeof(option_I) - 1] = '\0';
            }
        } else if (strcmp(token, "-P") == 0) {
            token = strtok(NULL, " ");
            if (token != NULL) {
                option_P = atoi(token);
            }
        } else if (strcmp(token, "-f") == 0) {
            token = strtok(NULL, " ");
            if (token != NULL) {
                strncpy(file_path, token, sizeof(file_path) - 1);
                file_path[sizeof(file_path) - 1] = '\0';
                // need to check if the file exists before sending it to server
                if(compute_kmeans && checkFileExists(file_path, &file_size)){
                    ERROR("Given filename is invalid!")
                    return;
                }
            }
        } else if (strcmp(token, "-k") == 0) {
            token = strtok(NULL, " ");
            if (token != NULL) {
                num_clusters = atoi(token);
            }
        } else if (strcmp(token, "-m") == 0) {
            token = strtok(NULL, " ");
            if (token != NULL) {
                max_randnum = atoi(token);
            }
        } else if (strcmp(token, "-h") == 0) {
            print_command_help(compute_inverse);
            return;
        } else if (strcmp(token, "-D") == 0) {
            // modern commands need modern help menus
            if(compute_inverse){
                printf("Default values for matinvpar: \n");
                printf("    -n %d\n", matrix_size);
                printf("    -I %s\n", option_I);
                printf("    -m %d\n", max_randnum);
                printf("    -P %d\n", option_P);
            } else{
                printf("Default values for kmeanspar: \n");
                printf("    -k %d\n", num_clusters);
            }
            return;
        }
        token = strtok(NULL, " ");
    }

    // validate matrix size for inverse-calculator
    if(compute_inverse && matrix_size<=0){
        ERROR("Invalid matrix size!")
        return;
    }
    // check if the user forgot to provide a data-file for k-means
    if(compute_kmeans && strcmp(file_path, "")==0){
        ERROR("Filename not given for kmeans-data!")
        return;
    }

    if(compute_inverse){
        printf("\nsize      = %dx%d ", matrix_size, matrix_size);
        printf("\nmaxnum    = %d \n", max_randnum);
        printf("Init	  = %s \n", option_I);
    }

    // initiate handshake with server to solve the input problem
    char formatted_command[MAX_COMMAND_LENGTH];
    sprintf(formatted_command, "command: %s matsize: %d I: %s P: %d max_randnum: %d num_clusters: %d filesize: %ld\n", main_command, matrix_size, option_I, option_P, max_randnum, num_clusters, file_size);
    if(send(server_socket, formatted_command, MAX_COMMAND_LENGTH, 0) < 0){
        perror("Coudn't send the handshake request!");
        return;
    }
    
    // have to send a file for k-means
    if(compute_kmeans){
        sendFile(file_path, server_socket);
        printf("Data sent. Waiting for server...\n");
    }

    // now complete the handshake by getting the output-file-size and request-counter
    char *reception = (char *)malloc(MAX_COMMAND_LENGTH);
    size_t output_filesize;
    int count;
    recv(server_socket, reception, MAX_COMMAND_LENGTH, 0);
    sscanf(reception, "request_count: %d output_filesize: %ld", &count, &output_filesize);
    if(output_filesize == 0){
        printf(MAGENTA"Server response: %s\n"NORMAL, reception);
        ERROR("Couldn't sync with server. Aborted.")
        return;
    }

    // receive and save the results in a unique file (BUT on restart the uniqueness disappears)
    char savefile_name[MAX_COMMAND_LENGTH];
    int file_count = 1;
    while(1){
        if(compute_inverse)
            sprintf(savefile_name, RESULTS_DIR"matinv_soln_%d.txt", file_count);
        else
            sprintf(savefile_name, RESULTS_DIR"kmeans_soln_%d.txt", file_count);
        
        if(access(savefile_name, F_OK) != 0)
            break;
        file_count++;
    }

    receiveFile(savefile_name, server_socket, output_filesize);
    // notify the user
    printf("Obtained results. Solution saved in: %s", savefile_name);
    free(reception);
}


int main(int argc, char **argv){
    // socket variables
    int sockfd;
    struct sockaddr_in server_addr;

    // creating the socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // parse the command line options
    parse_options(argc, argv);
    printf(MAGENTA"\nConnecting to server: %s:%d....\n"NORMAL, server_ip, connection_port);

    // Initialize server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(connection_port);
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        perror("\nInvalid address/ Address not supported");
        exit(EXIT_FAILURE);
    }

    // connect to the server
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("\nConnection failed");
        exit(EXIT_FAILURE);
    }
    // print a nice, green success message
    printf(GREEN"...Connected.\n\n"NORMAL);
    

    // loop forever, until the user gets tired of doing math
    while(1){
        // read the shell command
        char *command;
        command = readline(BLUE"\ncommand-shell> "NORMAL);
        
        // if uses enter Ctrl+D, we don't exit -- since the project said "only exit when Ctrl+C is pressed"
        if(command == NULL)
            continue;
        
        // add the history functionality to the shell (the arrow keys to bring old commands into view) -- very useful!!
        if(strlen(command)>0)
            add_history(command);
        
        // now parse the inputted command and execute it if possible
        parse_commands(command, sockfd);
        
        // free the space allocated to the command
        free(command);
    }

    // close the socket at the end
    close(sockfd);

    return EXIT_SUCCESS;
}
