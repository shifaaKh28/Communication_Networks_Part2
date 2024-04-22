#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "RUDP.API.h"

#define FILE_SIZE 2097152 // 2 MB

int main(int argc, char *argv[]) {
    // Check command line arguments
    if (argc != 3 || strcmp(argv[1], "-p") != 0) {
        fprintf(stderr, "Usage: %s -p <port>\n", argv[0]);
        return 1;
    }

    // Parse command line arguments
    int port = atoi(argv[2]);

    // Create a UDP socket
    int sockfd = rudp_socket();
    if (sockfd == -1) {
        perror("rudp_socket() failed");
        return 1;
    }
    printf("Created an RUDP socket\n");

    // Bind the socket to the specified port
    if (rudp_bind(sockfd, port) != 0) {
        perror("rudp_bind() failed");
        rudp_close(sockfd);
        return 1;
    }
    printf("Bound RUDP socket to port %d\n", port);

    // Declare a RUDP_Packet variable to hold the received packet
    RUDP_Packet received_packet;

    // Receive the file and measure time
    struct timeval start_time, end_time;
    gettimeofday(&start_time, NULL);

    // Receive the file using rudp_receive()
    int received_bytes = rudp_receive(sockfd, &received_packet);
    if (received_bytes < 0) {
        fprintf(stderr, "Error receiving packet.\n");
        rudp_close(sockfd);
        return 1;
    }

    gettimeofday(&end_time, NULL);

    // Calculate the time taken to receive the file
    double time_taken = (double)(end_time.tv_sec - start_time.tv_sec) * 1000 +
                        (double)(end_time.tv_usec - start_time.tv_usec) / 1000;

    // Calculate the bandwidth
    double bandwidth = (double)FILE_SIZE / (time_taken / 1000);

    // Print out statistics
    printf("----------------------------------\n");
    printf("- * Statistics * -\n");
    printf("- Time: %.2f ms\n", time_taken);
    printf("- Bandwidth: %.2f MB/s\n", bandwidth);
    printf("----------------------------------\n");

    // Close the socket
    rudp_close(sockfd);
    printf("Closed the RUDP socket\n");

    return 0;
}
