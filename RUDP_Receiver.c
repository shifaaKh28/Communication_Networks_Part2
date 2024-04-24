#include <arpa/inet.h>   // For manipulating IP addresses
#include <stdio.h>       // For standard input/output operations
#include <stdlib.h>      // For standard library functions
#include <string.h>      // For string manipulation functions
#include <sys/socket.h>  // For socket-related functions
#include <sys/time.h>    // For time-related functions
#include <time.h>        // For time-related functions
#include <unistd.h>      // For standard symbolic constants and types

#include "RUDP_API.h"    // Header file for the Reliable UDP (RUDP) API

#define PORT 1234        // Default port number
#define MAX_SIZE 1024*1024*2 // Size of the random data to generate (2MB)

/**
 * @brief Main function to receive data using the RUDP protocol.
 * @param argc Number of command-line arguments.
 * @param argv Array of command-line argument strings.
 * @return 0 on successful execution, -1 on failure.
 */
int main(int argc, char *argv[]) {
    // Check if the correct number of command-line arguments is provided
    if (strcmp(argv[1], "-p") != 0 || argc != 3  ) {
        printf("Invalid  input\n");
        return -1;
    }

    printf("Starting Receiver...\n");

    // Extract port number from command-line argument
    int port = atoi(argv[2]);  

    // Create a socket for receiving data
    int sockfd = rudp_socket();
    if (sockfd == -1) {
        printf("Failed to create the socket\n");
        return -1;
    }

    printf("Waiting for RUDP connection...\n");

    if (rudp_accept(sockfd, port) == 0) {
        printf("Failed connection\n");
        return -1;
    }

    printf("Connection request received, sending ACK.\n");

    // Open a file to store received data
    FILE *fp = fopen("recieved_data", "w+");
    if (fp == NULL) {
        printf("failed to open the file\n");
        return -1; 
    }

    printf("Sender connected, beginning to receive file...\n");

    // Variables for calculating average time and speed
    double average_time = 0;
    double average_bandwidth = 0;
    clock_t start, finish;

    // Buffers for receiving data
    char *recv_data = NULL;
    int data_len = 0;
    char total_size[MAX_SIZE] = {0};

    // Flags for tracking data reception status
    int data_flag = 0;
    int run = 1;

    start = clock(); 
    finish = clock();

    // Loop to receive data until connection is closed
    do {
        // Receive data packet
        data_flag = rudp_receive(sockfd, &recv_data, &data_len);

        // Check the received data state
        if (data_flag == -5) {
            break;  // Connection closed by sender
        } else if (data_flag == -1) {
            printf("Error receiving the data\n");
            return -1;
        } else if (data_flag == 1 && start < finish) {
            start = clock();  // Start timing for data transfer
        } else if (data_flag == 1) {
            strcat(total_size, recv_data);  // Append received data to total
        } else if (data_flag == 5) {
            finish = clock();  // Finish timing for data transfer
             //calculates the duration of the process in seconds with fractional precision.
            double elapsed_time = ((double)(finish - start)) / CLOCKS_PER_SEC;
            average_time += elapsed_time;

            double bandwidth = 2 / elapsed_time;  // Assuming data size is 2 MB
            average_bandwidth += bandwidth;

            // Write stats to the data file
            fprintf(fp, "Run #%d Data: Time=%.2fms Speed=%.2f MB/s\n", run, elapsed_time * 1000, bandwidth);

            // Reset buffers and counters for next run
            memset(total_size, 0, sizeof(total_size));
            run++;                     
        }
    } while (data_flag >= 0);

    printf("File transfer completed.\n");

    printf("ACK sent.\n");

    printf("Waiting for Sender response...\n");

    printf("Sender sent exit message.\n");

    printf("ACK sent.\n");
double average_time_ms = average_time * 1000;
double average_bandwidth_MBps = average_bandwidth;

printf("----------------------------------\n");
printf("- * Statistics * -\n");
printf("- Run #1 Data: Time=%.2fms; Speed=%.2f MB/s\n", average_time_ms, average_bandwidth_MBps);
printf("-\n");
printf("- Average time: %.2fms\n", (average_time_ms / (run - 1)));
printf("- Average bandwidth: %.2f MB/s\n", average_bandwidth_MBps / (run - 1));

    printf("----------------------------------\n");

    printf("Receiver end.\n");

    // Close the file
    fclose(fp);

    return 0;
}
