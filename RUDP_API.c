#include "RUDP_API.h" // Header file for the Reliable UDP (RUDP) API
#include <stdio.h> 
#include <stdlib.h> 
#include <stdbool.h>  // Standard boolean type and values
#include <string.h> 
#include <arpa/inet.h> // Definitions for internet operations
#include <unistd.h> // POSIX operating system API
#include <errno.h> // Error number definitions
#include <sys/socket.h> // Socket programming functions and structures
#include <sys/time.h> // Time-related functions and structures
#include <time.h> // Time functions
#include <sys/types.h> // Data types
#include <sys/socket.h>

// Initialize server address structure
struct sockaddr_in server_address;

int rudp_socket() {
    // Create a socket for UDP communication
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        perror("socket creation failed");
        return -1; // Return -1 to indicate failure
    }
    return sockfd; // Return the socket file descriptor
}

int rudp_send(int socket, const char *data, int size) {
    // Calculate the number of packets needed to send the data
    int number_packets = size / MAX_SIZE;
    int last_packet_size = size % MAX_SIZE;

    // Allocate memory for the RUDP packet
    RUDP_Packet *rudp = malloc(sizeof(RUDP_Packet));
    if (rudp == NULL) {
        perror("Memory allocation failed");
        return -1;
    }

    // Iterate through each packet to send
    for (int i = 0; i < number_packets; i++) {
        // Initialize the RUDP packet
        memset(rudp, 0, sizeof(RUDP_Packet));
        rudp->sequalNum = i;        // Set the sequence number
        rudp->flags.isData = 1;  // Set the data packet flag
        if (i == last_packet_size - 1 && last_packet_size == 0) {
            rudp->flags.fin = 1;  // Set the finish flag for the last packet
        }
        // Copy the data into the packet
        memcpy(rudp->data, data + (i * MAX_SIZE), MAX_SIZE);
        rudp->length = MAX_SIZE;  // Set the data length
        rudp->checksum = calculate_checksum(rudp);  // Calculate and set the checksum

        // Send the packet and wait for acknowledgement
        do {
            int res = sendto(socket, rudp, sizeof(RUDP_Packet), 0, NULL, 0);
            if (res == -1) {
                perror("Error sending with sendto");
                free(rudp);
                return -1;
            }
        } while (wait_for_acknowledgement(socket, i, clock(), 1) <= 0);
    }

    // Send the last packet if there's remaining data
    if (last_packet_size > 0) {
        // Initialize the last packet
        memset(rudp, 0, sizeof(RUDP_Packet));
        rudp->sequalNum = number_packets;  // Set the sequence number
        rudp->flags.isData = 1;         // Set the data packet flag
        rudp->flags.fin = 1;            // Set the finish flag

        // Copy the last portion of data into the packet
        memcpy(rudp->data, data + number_packets * MAX_SIZE, last_packet_size);
        rudp->length = last_packet_size;  // Set the data length
        rudp->checksum = checksum(rudp);    // Calculate and set the checksum

        // Send the last packet and wait for acknowledgement
        do {
            int send_last_packet = sendto(socket, rudp, sizeof(RUDP_Packet), 0, NULL, 0);
            if (send_last_packet == -1) {
                perror("Error : can't send the last packet");
                free(rudp);
                return -1;
            }
        } while (wait_ack(socket, number_packets, clock(), 1) <= 0);
    }

    // Free the allocated memory for the RUDP packet
    free(rudp);

    return 1;  // Return 1 on successful transmission
}

int seq_number = 0;
int rudp_receive(int socket, char **buffer, int *size) {
    // Allocate memory for the received RUDP packet
    RUDP_Packet *rudp = malloc(sizeof(RUDP_Packet));
     if (rudp == NULL) {
        perror("Memory allocation failed");
        return -1;
    }
    // Initialize the memory block to zero
    memset(rudp, 0, sizeof(RUDP_Packet));

    // Receive data from the socket into the allocated memory
    int recv_data = recvfrom(socket, rudp, sizeof(RUDP_Packet) - 1, 0, NULL, 0);
    if (recv_data == -1) {  
        // Handle the case when an error occurs during receiving
        perror("Error: failed recieving data");
        free(rudp);
        return -1; 
    }

    // Verify the checksum to ensure data integrity
    if (calculate_checksum(rudp) != rudp->checksum) {  
        free(rudp);
        return -1;
    }

    // Send acknowledgment for the received packet
    if (send_ack(socket, rudp) == -1) {
        free(rudp);
        return -1;
    }

    // Handle connection request
    if (rudp->flags.isSyn == 1) {  
        printf("there is a new request for connection\n");
        free(rudp);
        return 0;  // Connection request received
    }

    // Check if the received packet is expected based on sequence number
    if (rudp->sequalNum == seq_number) {
        // Handle regular data packet
        if (rudp->flags.isData == 1 && rudp->sequalNum == 0 ) {
            set_time(socket, 1 * 10);
        }
        // Handle the case when the entire message is received and the connection is closed
        if (  rudp->flags.isData == 1 && rudp->flags.fin == 1) {  
            *buffer = malloc(rudp->length);
            if (*buffer == NULL) {
            perror("Failed to allocate memory for the buffer");
            free(rudp);
            return -1;  // Error: Memory allocation failed
         }
             // Copy the received data into the buffer
            memcpy(*buffer, rudp->data, rudp->length);
            *size = rudp->length;
            free(rudp);
            seq_number = 0; 
            set_time(socket, 10000000);
            return 5;  // Message received and connection closed
        }

        // Handle regular data packet
        if (rudp->flags.isData == 1) {  
            *buffer = malloc(rudp->length);
              if (*buffer == NULL) {
              perror("Failed to allocate memory for the buffer");
              free(rudp);
              return -1;  // Error: Memory allocation failed
    }
            memcpy(*buffer, rudp->data, rudp->length);
            *size = rudp->length; 
            free(rudp);
            seq_number++;  
            return 1;  // Regular data packet received
        }
    }
    // Handle unexpected or out-of-order data packet
    else if (rudp->flags.isData == 1) {
        free(rudp);
        return 0;  // Unexpected or out-of-order data packet
    }

    // Handle closing connection
    if (rudp->flags.fin == 1) {  
        // Process closing connection
        free(rudp);
        printf("received close connection\n");
        set_time(socket, 1 * 10);
        rudp = malloc(sizeof(RUDP_Packet));
        time_t finishing= time(NULL);


        // Wait for acknowledgement for the closing packet
        while ((double)(time(NULL) - finishing) < 1) {
            memset(rudp, 0, sizeof(RUDP_Packet));
            recvfrom(socket, rudp, sizeof(RUDP_Packet) - 1, 0, NULL, 0);
            if (rudp->flags.fin == 1) {
                if (send_acknowledgement(socket, rudp) == -1) { 
                    free(rudp);
                    return -1;  // Error sending acknowledgment for closing packet
                }
                finishing = time(NULL);
            }
        }

        free(rudp);
        close(socket);
        return -5;  // Connection closed
    }
    free(rudp);
    return 0;  // No data received or unexpected packet type
}

int rudp_connect(int socket, const char *ip,unsigned short int port) {
    // Set timeout for the socket
    if (set_time(socket, 1) == -1) {
        return -1;
    }


    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;  // IPv4
    server_address.sin_port = htons(port);  // Convert port to network byte order

    // Convert IP address to binary form and store it in the server address structure
    int val = inet_pton(AF_INET, ip, &server_address.sin_addr);
    if (val <= 0) {
        return -1;
    }

    // Connect to the remote server
    if (connect(socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
        perror("Error: Connection failed");
        return -1;
    }

    // Prepare synchronization packet
    RUDP_Packet *rudp = malloc(sizeof(RUDP_Packet));
         if (rudp == NULL) {
        perror("Memory allocation failed");
        return -1;
    }
    memset(rudp, 0, sizeof(RUDP_Packet));
    rudp->flags.isSyn = 1;
    int attempts = 0; //connection attempts
    RUDP_Packet *recv = NULL;

    // Attempt to establish connection
    while (attempts < 3) {
        // Send synchronization packet to the server
        int sendRes = sendto(socket, rudp, sizeof(RUDP_Packet), 0, NULL, 0);
        if (sendRes == -1) {
            perror("failed to send the data");
            free(rudp);
            return -1;
        }

        clock_t st = clock(); // Start timer for response

        do {
            // Receive response from the server
            recv = malloc(sizeof(RUDP_Packet));
            memset(recv, 0, sizeof(RUDP_Packet));
            int get_data = recvfrom(socket, recv, sizeof(RUDP_Packet), 0, NULL, 0);

            if (get_data == -1) {
                perror("failed to receive the  data");
                free(rudp);
                free(recv);
                return -1;
            }

            // Check if the received packet indicates successful connection
            if (recv->flags.isSyn && recv->flags.ack) {
                printf("Connect successfully\n");
                free(rudp);
                free(recv);
                return 1;
            } else {
                printf("unvalid packet is recieved\n");
            }
        } while ((double)(clock() - st) / CLOCKS_PER_SEC < 1); // Wait for response for 1 second
        attempts++; // Increment the number of connection attempts
    }

    printf("Failed  connection after many attempts to connect\n");
    free(rudp);
    free(recv);
    return 0; // Connection attempt failed
}
// unsigned short int checksum(RUDP_Packet *rudp) {
//   int sum = 0;
//   for (int i = 0; i < 10 && i < MAX_SIZE; i++) {
//     sum += rudp->data[i];
//   }
//   return sum;
// }

unsigned short int calculate_checksum(RUDP_Packet *rudp) {
    unsigned int sum = 0;
    unsigned short *buf = (unsigned short *)rudp;
    unsigned short checksum;

    // Calculate the sum of all 16-bit words
    for (int i = 0; i < sizeof(RUDP_Packet) / 2; i++) {
        sum += *buf++;
    }

    // Add carry bits to the sum
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    // Take the one's complement
    checksum = ~sum;

    return checksum;
}
int send_ack(int sockfd, RUDP_Packet *received_packet){
  // Allocate memory for the acknowledgment packet
    RUDP_Packet *ack_packet = malloc(sizeof(RUDP_Packet));
    if (ack_packet == NULL) {
        perror("Failed to allocate memory for acknowledgment packet");
        return -1;  // Error occurred
    }
  // Initialize acknowledgment packet flags
    if (received_packet->flags.isData == 1) {
        ack_packet->flags.isData = 1;  // Indicates a data packet
    }
  // Set acknowledgment packet flags based on the received RUDP packet
    if (received_packet->flags.fin == 1) {
        ack_packet->flags.fin = 1;  // Indicates completion of data transmission
    }
   // Set the sequence number and calculate the checksum
    ack_packet->sequalNum = received_packet->sequalNum;
    ack_packet->checksum = calculate_checksum(ack_packet);

    // Transmit the acknowledgment packet
    int send_result = sendto(socket, ack_packet, sizeof(RUDP_Packet), 0, NULL, 0);
    if (send_result == -1) {
        perror("Error: transmit acknowledgment");
        free(ack_packet);
        return -1;  // Transmission error
    }

    // Free memory allocated for the acknowledgment packet
    free(ack_packet);

    // Return success
    return 1;  
}

int wait_ack(int sockfd, int seq_num, clock_t st, clock_t tout) {
    // Allocate memory for the acknowledgment packet
    RUDP_Packet *ack_packet = malloc(sizeof(RUDP_Packet));
    if (ack_packet == NULL) {
        perror("Failed to allocate memory for acknowledgment packet");
        return -1;  // Error occurred
    }

    // Loop until timeout
    clock_t elapsed_time;
    double elapsed_seconds;
    do {
        elapsed_time = clock() - st;
        elapsed_seconds = (double)elapsed_time / CLOCKS_PER_SEC;

        // Receive packet from the socket
        int length = recvfrom(sockfd, ack_packet, sizeof(RUDP_Packet), 0, NULL, 0);
        if (length == -1) {
            perror("Failed to receive acknowledgment packet");
            free(ack_packet);
            return -1;  // Error occurred
        }

        // Check if the received packet matches the expected sequence number and is an acknowledgment
        if (ack_packet->sequalNum == seq_num && ack_packet->flags.ack == 1) {
            free(ack_packet);
            return 1;  // Acknowledgment received
        }
    } while (elapsed_seconds < tout);

    free(ack_packet);
    return 0;  // Timeout reached without acknowledgment
}




