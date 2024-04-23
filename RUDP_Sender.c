#include <arpa/inet.h>   // For manipulating IP addresses
#include <stdbool.h>     // For boolean data type
#include <stdio.h>       // For standard input/output operations
#include <stdlib.h>      // For standard library functions
#include <string.h>      // For string manipulation functions
#include <sys/socket.h>  // For socket-related functions
#include <sys/time.h>    // For time-related functions
#include <time.h>        // For time-related functions
#include <unistd.h>      // For standard symbolic constants and types

#include "RUDP_API.h"    // Header file for the Reliable UDP (RUDP) API

#define PORT 1234        // Default port number
#define IP "127.0.0.1"  // Default server IP address
#define MAX_SIZE 1024*1024*2 // Size of the random data to generate (2MB)

/**
 * @brief A random data generator function based on srand() and rand().
 * @param size The size of the data to generate (up to 2^32 bytes).
 * @return A pointer to the buffer containing the generated data.
 */
char *util_generate_random_data(unsigned int size) {
    char *buffer = NULL;
    // Argument check.
    if (size == 0)
        return NULL;
    buffer = (char *)calloc(size, sizeof(char));
    // Error checking.
    if (buffer == NULL)
        return NULL;
    // Randomize the seed of the random number generator.
    srand(time(NULL));
    for (unsigned int i = 0; i < size; i++)
        *(buffer + i) = ((unsigned int)rand() % 256);
    return buffer;
}

/**
 * @brief Main function to send data using the RUDP protocol.
 * @param argc Number of command-line arguments.
 * @param argv Array of command-line argument strings.
 * @return 0 on successful execution, 1 on failure.
 */
int main(int argc, char *argv[]) {
    char *ip;
    int port_number;

    if (argc != 5 || strcmp(argv[1], "-ip") != 0 || strcmp(argv[3], "-p") != 0) {
        printf("invalid  input\n");
        return 1;
    }
    ip = argv[2];
    char *port = argv[4];
    char *ptr;

    int input_port = strtol(port, &ptr, 10);
    if (*ptr != '\0' || input_port < 0 ) {
        printf("invalid port\n");
        return 1;
    }
    port_number = (int)input_port;

    char *data = util_generate_random_data(MAX_SIZE);

    // Create a UDP socket and establish a connection with the server
    int socket = rudp_socket();  
    if (socket == -1) {
    fprintf(stderr, "Error: Failed to create RUDP socket.\n");  
        free(data);
        return 1;  
    }
    if (rudp_connect(socket, ip, port_number) <= 0) {
    fprintf(stderr, "Error: Failed to create RUDP connect.\n");
        rudp_close(socket);
        free(data);
        return 1;
    }

    char option;
    do {
        printf("start Sending the data...\n");
        if (rudp_send(socket, data, MAX_SIZE) < 0) {
            printf("failed to send the data...\n");
            rudp_close(socket);
            free(data);
            return 1;
        }
        printf("Do you want to send it again? (y/n): \n");
        scanf(" %c", &option);
    } while (option == 'y');

    printf("Close connection...\n");
    rudp_close(socket);

    printf("Connection is closed\n");
    free(data);

    return 0;
}
