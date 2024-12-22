## Overview
`Server_in_C` is a server-client application written purely in C, and designed to handle multiple client connections simultaneously. The server uses sockets for network communication and supports parallel processing of requests using `fork()` to spawn a new process for each independent client. This demo-server handles any client-side requests asking for a solution to numerical matrix-inversion and k-means clustering problems.

## Features
- **Multiple Client Connections:** The server can handle multiple clients simultaneously using the `fork()` system call, creating a new process for each client.
- **Socket Communication:** Utilizes TCP sockets for reliable communication between the server and clients.
- **Command Handling:** The server processes specific commands from clients for matrix inversion and k-means clustering.
- **File Transfer:** Supports receiving data files from clients and sending result files back to clients.
- **Logging:** Logs runtime errors and significant events to log files.
- **Configurable Port:** The server can be started on any specified port, making it flexible for various network environments.

## Requirements
- GCC (GNU Compiler Collection)
- Make
- Linux environment (recommended)
- `readline` library for client-side command-line interface

## Installation
To compile and run the server and client, follow these steps:

1. Clone the repository:
   ```sh
   git clone https://github.com/TheSUPERCD/Server_in_C.git
   ```

2. Navigate to the project directory:
   ```sh
   cd Server_in_C
   ```

3. Compile the server and client using `make`:
   ```sh
   make
   ```

## Usage

### Starting the Server
To start the server, run the following command:
```sh
./server -p <port>
```
Replace `<port>` with the port number you want the server to listen on.

Example:
```sh
./server -p 8080
```

### Connecting with the Client
To start the client and connect to the server, run the following command:
```sh
./client -ip <server_ip> -p <server_port>
```
Replace `<server_ip>` with the server's IP address and `<server_port>` with the port number the server is listening on.

Example:
```sh
./client -ip 127.0.0.1 -p 8080
```

### Client Interaction
Once connected, clients can send commands to the server via a command-line interface. The client supports history and auto-completion for ease of use.

### Supported Commands
The server processes the following commands:

- **Matrix Inversion (`matinvpar`):**
  This command computes the inverse of a given matrix.

  Usage:
  ```sh
  matinvpar -n <matrix_size> -I <init_type> -P <print_switch> -m <max_randnum>
  ```
  Example:
  ```sh
  matinvpar -n 4 -I fast -P 1 -m 10
  ```

- **K-Means Clustering (`kmeanspar`):**
  This command performs k-means clustering on a dataset.

  Usage:
  ```sh
  kmeanspar -f <file_path> -k <num_clusters>
  ```
  Example:
  ```sh
  kmeanspar -f ./data/kmeans-data.txt -k 3
  ```
  The client will then send the data file to the server.

- **Shutdown:**
  This command gracefully shuts down the server process auto-spawned for the client.

  Usage:
  ```sh
  shutdown
  ```

### Server Options
- **-h:** Print the help message.
- **-p port:** Listen on the specified port number (defaults to port 8080).
- **-d:** Run as a daemon.
- **-s strategy:** Specify the request handling strategy (only the `fork` strategy is implemented, the program defaults to this setting).

Example:
```sh
./server -p 8080 -s fork
```

### Client Options
- **-h:** Print the help message.
- **-ip server_ip:** Specify the server IP address (defaults to localhost ip).
- **-p server_port:** Specify the server port number (defaults to port 8080).

Example:
```sh
./client -ip 127.0.0.1 -p 8080
```

## Acknowledgments
- Thanks to **Microsoft** and **GitHub Copilot** for providing a great template for this `README.md` file.

