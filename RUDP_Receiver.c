#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include "RUDP.API.h"

#define BUFFER_SIZE 8192
#define EXIT_MESSAGE "exit"

int main(int argc, char *argv[]) {
    // Check command-line arguments
    if (argc != 3) {
        fprintf(stderr, "Usage: %s -p <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Parse command-line arguments
    int port = atoi(argv[2]);

    // Create a UDP socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Bind the socket to the specified port
    struct sockaddr_in receiver_addr;
    memset(&receiver_addr, 0, sizeof(receiver_addr));
    receiver_addr.sin_family = AF_INET;
    receiver_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    receiver_addr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr *)&receiver_addr, sizeof(receiver_addr)) < 0) {
        perror("bind");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Starting Receiver...\n");

    // Wait for RUDP connection
    printf("Waiting for RUDP connection...\n");
    int sockfd  = rudp_socket();
    if (rudp_socket == NULL) {
        perror("rudp_socket");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Accept connection from sender using RUDP protocol
    printf("Accepting connection from Sender...\n");
    if (rudp_accept(rudp_socket) < 0) {
        perror("rudp_accept");
        rudp_close(rudp_socket);
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    printf("Sender connected.\n");

    // Receive the file, measure time, and save it
    struct timeval start_time, end_time;
    gettimeofday(&start_time, NULL);

    FILE *file = fopen("received_file", "wb");
    if (file == NULL) {
        perror("fopen");
        rudp_close(rudp_socket);
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Receiving file...\n");
    ssize_t bytes_received;
    char buffer[BUFFER_SIZE];
    while ((bytes_received = rudp_receive(rudp_socket, buffer)) > 0) {
        fwrite(buffer, 1, bytes_received, file);
    }
    fclose(file);

    gettimeofday(&end_time, NULL);

    long long int start_ms = start_time.tv_sec * 1000LL + start_time.tv_usec / 1000;
    long long int end_ms = end_time.tv_sec * 1000LL + end_time.tv_usec / 1000;
    long long int elapsed_time = end_ms - start_ms;

    printf("File received successfully in %lld ms.\n", elapsed_time);

    // Wait for Sender response
    printf("Waiting for Sender response...\n");
    char response[BUFFER_SIZE];
    bytes_received = rudp_receive(rudp_socket, response);
    if (bytes_received < 0) {
        perror("rudp_receive");
    } else if (bytes_received == 0) {
        printf("Sender sent exit message.\n");
    } else {
        printf("Unexpected response from Sender: %s\n", response);
    }

    // Print out statistics
    // Calculate average time and bandwidth
    // Implement statistics printing logic

    // Exit
    printf("Receiver end.\n");
    rudp_close(rudp_socket);
    close(sockfd);
    return 0;
}
