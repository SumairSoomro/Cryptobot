#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>

// Function to create a socket
int create_socket() {
    // Create a socket and get the file descriptor
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    
    // Check if socket creation was successful
    if (clientfd < 0) {
        fprintf(stderr, "Failed to create socket: %s\n", strerror(errno));
    }
    
    return clientfd;
}

// Function to configure the server address
void configure_server_address(struct sockaddr_in *serveraddr, char *hostname, int port) {
    // Initialize the server address struct
    bzero((char *)serveraddr, sizeof(*serveraddr));
    
    // Set the address family as IPv4
    serveraddr->sin_family = AF_INET;
    
    // Set the port number, converting from host byte order to network byte order
    serveraddr->sin_port = htons(port);

    // Convert the hostname to an IP address and store in server address struct
    if (inet_pton(AF_INET, hostname, &(serveraddr->sin_addr)) <= 0) {
        fprintf(stderr, "Failed to convert IP address: %s\n", strerror(errno));
    }
}

// Function to establish a connection to the server
int connect_to_server(int clientfd, struct sockaddr_in *serveraddr) {
    // Connect to the server using the socket file descriptor and server address struct
    if (connect(clientfd, (struct sockaddr *)serveraddr, sizeof(*serveraddr)) < 0) {
        fprintf(stderr, "Failed to connect: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

// Updated open_clientfd() function using the new functions
int open_clientfd(char *hostname, int port) {
    struct sockaddr_in serveraddr;

    // Step 1: Create a socket
    int clientfd = create_socket();
    if (clientfd < 0) {
        return -1;
    }

    // Step 2: Configure the server address
    configure_server_address(&serveraddr, hostname, port);

    // Step 3: Connect to the server
    if (connect_to_server(clientfd, &serveraddr) < 0) {
        close(clientfd);
        return -1;
    }

    return clientfd;
}

// Function to decrypt a message using a given ciphertext alphabet
char* decrypt_message(char* cipher, char* message) {
    // Allocate memory for the decrypted message
    char *decrypt = (char *)malloc(strlen(message) + 1);
    
    // Decrypt each character in the message by mapping it to the cipher
    for (int i = 0; i < strlen(message); ++i) {
        decrypt[i] = cipher[((int)message[i]) - 97]; // -97 to get the zero-based index
    }
    
    // Null terminate the decrypted message
    decrypt[strlen(message)] = '\0';
    
    return decrypt;
}

int main(int argc, char *argv[]) {
    // Get the arguments from the command line
    char *identification = argv[1];
    int port = atoi(argv[2]);
    char *hostname = argv[3];

    // Open a connection to the server
    int clientfd = open_clientfd(hostname, port);

    // Check if connection was successful
    if (clientfd < 0) {
        fprintf(stderr, "Failed to connect to the server\n");
        return EXIT_FAILURE;
    }

    // A buffer to receive data from the server.
    char buf[8192];

    // Send a hello message to the server
    sprintf(buf, "cs230 HELLO %s)\n", identification);
    ssize_t send_to;
    ssize_t receive_from;

    // Send the buffer to the server
    send_to = send(clientfd, buf, strlen(buf), 0);
    if (send_to < 0) {
        perror("send");
        close(clientfd);
        return EXIT_FAILURE;
    }

    // Clear the buffer
    bzero(buf, sizeof(buf));

    // Receive a response from the server
    receive_from = recv(clientfd, buf, 8192, 0);
    if (receive_from < 0) {
        perror("receive");
        close(clientfd);
        return EXIT_FAILURE;
    }

    // Null terminate the received data
    buf[receive_from] = '\0';

    // Loop until we receive a 'BYE' message from the server
    while (1) {
        // Extract the cipher and the encrypted message from the buffer
        char cipher[27];
        memcpy(cipher, &buf[13], 26);
        cipher[26] = '\0';
        char message[strlen(buf) - 39];
        memcpy(message, &buf[40], strlen(buf) - 40);
        message[strlen(buf) - 39] = '\0';

        // Decrypt the message
        char *decrypt = decrypt_message(cipher, message);

        // Clear the buffer
        bzero(buf, sizeof(buf));

        // Send the decrypted message back to the server
        sprintf(buf, "cs230 %s\n", decrypt);
        send_to = send(clientfd, buf, strlen(buf), 0);
        if (send_to < 0) {
            perror("send");
            free(decrypt);
            close(clientfd);
            return EXIT_FAILURE;
        }

        // Clear the buffer
        bzero(buf, sizeof(buf));

        // Receive a response from the server
        receive_from = recv(clientfd, buf, 8192, 0);
        if (receive_from < 0) {
            perror("receive");
            free(decrypt);
            close(clientfd);
            return EXIT_FAILURE;
        }

        // Null terminate the received data
        buf[receive_from] = '\0';

        // Free the memory for the decrypted message
        free(decrypt);

        // Check if we received a 'BYE' message from the server
        if (buf[strlen(buf) - 4] == 'B' && buf[strlen(buf) - 3] == 'Y' && buf[strlen(buf) - 2] == 'E') {
            break;
        }
    }

    // Extract the flag from the buffer
    char flag[strlen(buf) - 10];
    memcpy(flag, &buf[6], strlen(buf) - 11);
    flag[strlen(buf) - 9] = '\0';
    
    // Print the flag
    printf("Flag: %s\n", flag);

    // Close the client socket file descriptor.
    close(clientfd);
    return EXIT_SUCCESS;
}
