#include <stdio.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include "RUDP.API.h"
// Define a constant for the data flag
#define DATA 1


#define MSG_BUFFER_SIZE 8192 // Define your own message buffer size if needed
#define FILE_SIZE 2097152    // 2 MB
#define SERVER_IP "127.0.0.1"
#define PORT 1234

// Function to generate random data
char *util_generate_random_data(unsigned int size) {
    char *buffer = (char *)malloc(size);
    if (buffer == NULL) {
        perror("malloc failed");
        return NULL;
    }
    for (unsigned int i = 0; i < size; i++) {
        buffer[i] = rand() % 256;
    }
    return buffer;
}

int main(int argc, char *argv[]) {
    // Check command line arguments
    if (argc != 5 ) {
        return 1;
    }

    // Generate random data
    char *message = util_generate_random_data(FILE_SIZE);
    if (message == NULL) {
        perror("util_generate_random_data() failed");
        return 1;
    }
    printf("Generated %d bytes of random data\n", FILE_SIZE);

    // Create a UDP socket
    int sockfd = rudp_socket();
    if (sockfd == -1) {
        perror("rudp_socket() failed");
        free(message);
        return 1;
    }
    printf("Created an RUDP socket\n");

    // Connect to the server
    if (rudp_connect(sockfd, argv[2], atoi(argv[4])) <= 0) {
        perror("rudp_connect() failed");
        rudp_close(sockfd);
        free(message);
        return 1;
    }
    printf("Connected to server at %s:%s\n", argv[2], argv[4]);

    char option = 'y';
    while (option == 'y') {
        // Send the data
        // int bytes_sent = rudp_send(sockfd, message, FILE_SIZE);
        int bytes_sent = rudp_send(sockfd, DATA, message, FILE_SIZE);

        if (bytes_sent == -1) {
            perror("rudp_send() failed");
            rudp_close(sockfd);
            free(message);
            return 1;
        }
        printf("Sent %d bytes to the server\n", bytes_sent);

        // Prompt user to send again
        printf("Do you want to send the message again? (y/n): ");
        scanf(" %c", &option);
    }

    // Close the socket
    rudp_close(sockfd);
    printf("Closed the RUDP socket\n");
    free(message);
    return 0;
}
