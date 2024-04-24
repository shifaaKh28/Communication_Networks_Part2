
/**
 * Wasim
 * Shifaa
*/
#include "RUDP_API.h"
#include <arpa/inet.h>  // For functions like inet_pton
#include <errno.h>      // For error handling
#include <stdio.h>      // For standard I/O operations
#include <stdlib.h>     // For dynamic memory allocation and other standard functions
#include <string.h>     // For string manipulation functions
#include <sys/socket.h> // For socket related functions
#include <sys/time.h>   // For time related functions
#include <sys/types.h>  // For data types
#include <time.h>       // For time related functions
#include <unistd.h>     // For POSIX operating system API

// Struct to hold server address information
struct sockaddr_in server_address;

// Struct to hold client address information
struct sockaddr_in client_address;

//struct Timeout value for socket operations.
 struct timeval timeout;

int rudp_socket() {
    // Create a new UDP socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    // Check if socket creation failed
    if (sockfd == -1) {
        perror("Socket creation failed");
        return -1;
    }
    return sockfd;
}

int rudp_send(int socket, const char *data, int size) {
    // Calculate the number of packets and size of the last packet
    int packets = size / MAX_PACK_SIZE;
    int last_pack_size = size % MAX_PACK_SIZE;

    // Allocate memory for the RUDP packet
    RUDP_Packet *rudp = malloc(sizeof(RUDP_Packet));
    if (rudp == NULL) {
        perror("Failed to allocate memory for RUDP packet");
        return -1;
    }

    // Loop through each packet
    for (int i = 0; i < packets; i++) {
        memset(rudp, 0, sizeof(RUDP_Packet));
        rudp->sequalNum = i;
        rudp->flags.isData = 1;
        if (i == packets - 1 && last_pack_size == 0) {
            rudp->flags.fin = 1;
        }
        memcpy(rudp->data, data + i * MAX_PACK_SIZE, MAX_PACK_SIZE);
        rudp->length = MAX_PACK_SIZE;
        rudp->checksum = calculate_checksum(rudp);
        
        // Send the packet and wait for acknowledgment
        do {
            int send_data = sendto(socket, rudp, sizeof(RUDP_Packet), 0, NULL, 0);
            if (send_data == -1) {
                perror("can't send the data");
                free(rudp);
                return -1;
            }
        } while (waiting_ack(socket, i, clock(), 1) <= 0);
    }
    
    // Handle the last packet
    if (last_pack_size > 0) {
        memset(rudp, 0, sizeof(RUDP_Packet));
        rudp->sequalNum = packets;
        rudp->flags.isData = 1;
        rudp->flags.fin = 1;
        memcpy(rudp->data, data + (packets * MAX_PACK_SIZE), last_pack_size);
        rudp->length = last_pack_size;
        rudp->checksum = calculate_checksum(rudp);
        while (waiting_ack(socket, packets, clock(), 1) <= 0) {
            int last_pack = sendto(socket, rudp, sizeof(RUDP_Packet), 0, NULL, 0);
            if (last_pack == -1) {
                perror("can't send the last packet");
                free(rudp);
                return -1;
            }
        }
    }
    
    // Free the allocated memory for the RUDP packet
    free(rudp);

    return 1;
}

// Global variable to track the sequence number
int seq_number = 0;

int rudp_receive(int socket, char **buffer, int *size) {
    // Allocate memory for the RUDP packet
    RUDP_Packet *rudp = malloc(sizeof(RUDP_Packet));
    if (rudp == NULL) {
        perror("Failed to allocate memory for RUDP packet");
        return -1;
    }
    memset(rudp, 0, sizeof(RUDP_Packet));
    

    timeout.tv_sec = 5;  // Set timeout to 5 seconds
    timeout.tv_usec = 0;
    if (setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        perror("Error setting timeout");
        free(rudp);
        return -1;
    }

    // Receive packet from socket
    if (recvfrom(socket, rudp, sizeof(RUDP_Packet) - 1, 0, NULL, 0) == -1) {
        perror("Failed to receive data");
        free(rudp);
        return -1;
    }

    // Reset timeout value for the socket
    timeout.tv_sec = 0;  // Set timeout to 0 seconds
    timeout.tv_usec = 0;
    if (setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        perror("Error resetting timeout");
        free(rudp);
        return -1;
    }
    
    // Send acknowledgment
    if (sending_ack(socket, rudp) == -1) {
        free(rudp);
        return -1;
    }

    // Verify checksum
    if (calculate_checksum(rudp) != rudp->checksum) {
        free(rudp);
        return -1;
    }
 
    // Handle connection request
    if (rudp->flags.isSyn == 1) {
        printf("Connection request received\n");
        free(rudp);
        return 0;
    }
    
    // Handle data packet
    if (rudp->sequalNum == seq_number) {
        if (rudp->sequalNum == 0 && rudp->flags.isData == 1) {
            // Set timeout for subsequent data packets
            timeout.tv_sec = 1;  // Set timeout to 5 seconds
            timeout.tv_usec = 0;
            if (setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
                perror("Error setting timeout");
                free(rudp);
                return -1;
            }
        }
        if (rudp->flags.fin == 1 && rudp->flags.isData == 1) {
            *buffer = malloc(rudp->length);
            if (*buffer == NULL) {
                perror("Failed to allocate memory for buffer");
                free(rudp);
                return -1;
            }
            memcpy(*buffer, rudp->data, rudp->length);
            *size = rudp->length;
            free(rudp);
            seq_number = 0;
            // Reset timeout value for the socket
            timeout.tv_sec = 0;  // Set timeout to 0 seconds
            timeout.tv_usec = 0;
            if (setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
                perror("Error resetting timeout");
                return -1;
            }
            return 5;
        }
        if (rudp->flags.isData == 1) {
            *buffer = malloc(rudp->length);
            if (*buffer == NULL) {
                perror("Failed to allocate memory for buffer");
                free(rudp);
                return -1;
            }
            memcpy(*buffer, rudp->data, rudp->length);
            *size= rudp->length;
            free(rudp);
            seq_number++;
            return 1;
        }
    }
    
    // Handle invalid packet
    else if (rudp->flags.isData == 1) {
        free(rudp);
        return 0;
    }
    
    // Handle connection close
    if (rudp->flags.fin == 1) {
        free(rudp);
        printf("Connection closed by sender\n");
        // Set timeout for subsequent packets
        timeout.tv_sec = 5;  // Set timeout to 5 seconds
        timeout.tv_usec = 0;
        if (setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
            perror("Error setting timeout");
            return -1;
        }
        rudp = malloc(sizeof(RUDP_Packet));
        if (rudp == NULL) {
            perror("Failed to allocate memory for RUDP packet");
            return -1;
        }
        time_t finishing = time(NULL);
        printf("Waiting for the statictics...\n");

        while ((double)(time(NULL) - finishing) < 1) {
            memset(rudp, 0, sizeof(RUDP_Packet));
            recvfrom(socket, rudp, sizeof(RUDP_Packet) - 1, 0, NULL, 0);
            if (rudp->flags.fin == 1) {
                if (sending_ack(socket, rudp) == -1) {
                    free(rudp);
                    return -1;
                }
                finishing = time(NULL);
            }
        }
        free(rudp);
        close(socket);
        return -5;
    }
    
    free(rudp);
    return 0;
}


int rudp_connect(int socket, const char *ip,unsigned short int port) {
    // Set timeout for socket operations

    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    if (setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        perror("Error setting timeout");
        return -1;
    }
    
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    
    // Convert IP address from text to binary form
    int val = inet_pton(AF_INET, ip, &server_address.sin_addr);
    if (val <= 0) {
        perror("Invalid IP address");
        return -1;
    }
    
    // Connect to the remote socket
    if (connect(socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
        perror("Connection failed");
        return -1;
    }
    
    // Send synchronization packet to establish connection
    RUDP_Packet *rudp = malloc(sizeof(RUDP_Packet));
    if (rudp == NULL) {
        perror("Memory allocation failed");
        return -1;
    }
    memset(rudp, 0, sizeof(RUDP_Packet));
    rudp->flags.isSyn = 1;

    int attempts = 0;
    // Attempt to establish connection with retries
    while (attempts < 3) {
        int sendRes = sendto(socket, rudp, sizeof(RUDP_Packet), 0, NULL, 0);
        if (sendRes == -1) {
            perror("Failed to send synchronization packet");
            free(rudp);
            return -1;
        }
        // Wait for acknowledgment packet with timeout
        time_t start_time = time(NULL);
        while ((time(NULL) - start_time) < 1) {
            RUDP_Packet *recv = malloc(sizeof(RUDP_Packet));
            memset(recv, 0, sizeof(RUDP_Packet));
            if (recvfrom(socket, recv, sizeof(RUDP_Packet), 0, NULL, 0) == -1) {
                perror("Failed receiving the data");
                free(rudp);
                free(recv);
                return -1;
            }
            // Check if valid acknowledgment received
            if (recv->flags.isSyn && recv->flags.ack) {
                printf("Connection established successfully\n");
                free(rudp);
                free(recv);
                return 1;
            } else {
                printf("Invalid packet received\n");
            }
        }
        attempts++;
    }
    printf("Error :Failed to connect after many attempts\n");
    free(rudp);
    return 0;
}

int rudp_accept(int socket, unsigned short int port) {
    // Initialize server address structure
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    // Bind the socket to the specified port
    if (bind(socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
        perror("Binding failed");
        close(socket);
        return -1;
    }
    socklen_t len = sizeof(client_address);
    memset((char *)&client_address, 0, sizeof(client_address));
    // Receive synchronization packet from client
    RUDP_Packet *rudp = malloc(sizeof(RUDP_Packet));
    memset(rudp, 0, sizeof(RUDP_Packet));
    if (recvfrom(socket, rudp, sizeof(RUDP_Packet) - 1, 0, (struct sockaddr *)&client_address, &len) == -1) {
        perror("Failed to receive data");
        free(rudp);
        return -1;
    }
    // Connect to the client
    if (connect(socket, (struct sockaddr *)&client_address, len) == -1) {
        perror("Connection failed");
        free(rudp);
        return -1;
    }
    // Send acknowledgment to client
    if (rudp->flags.isSyn == 1) {
        RUDP_Packet *reply = malloc(sizeof(RUDP_Packet));
        memset(reply, 0, sizeof(RUDP_Packet));
        reply->flags.isSyn = 1;
        reply->flags.ack = 1;
        int send_res = sendto(socket, reply, sizeof(RUDP_Packet), 0, NULL, 0);
        if (send_res == -1) {
            perror("Failed to send data");
            free(rudp);
            free(reply);
            return -1;
        }
        // Set timeout for socket operations
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;
        if (setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
            perror("Error setting timeout");
            return -1;
        }
        free(rudp);
        free(reply);
        return 1;
    }
    return 0;
}


int rudp_close(int socket) {
  RUDP_Packet *temp = (RUDP_Packet*) malloc(sizeof(RUDP_Packet));
  if (temp == NULL){
    perror("failed to allocate memory for closing socket");
    return -1;
  }
  memset(temp, 0, sizeof(RUDP_Packet));
  temp->flags.fin = 1;  // Finished so closing the connection
  temp->checksum = calculate_checksum(temp);
  temp->sequalNum = -1;
  while (waiting_ack(socket, -1, clock(), 1) <= 0) {
    if (sendto(socket, temp, sizeof(RUDP_Packet), 0, NULL, 0) == -1) {
      perror("Fialed sendto when closing");
      free(temp);
      return -1;  // for error
    }
  }
  close(socket);
  free(temp);
  return 1;  // succeeded to close the socket and freeing our rudp struct
}


int calculate_checksum(RUDP_Packet *rudp) {
    // Simple checksum calculation based on packet length
    int sum = 0;
    sum += rudp->length;
    return sum;
}


int waiting_ack(int socket, int sequal_num, clock_t s, clock_t t) {
  RUDP_Packet *temp = (RUDP_Packet*) malloc(sizeof(RUDP_Packet));
  if (temp == NULL){
    fprintf(stderr, "error allocating memory for sending ack");
    return -1;
  }
  while ((double)(clock() - s) / CLOCKS_PER_SEC < 1) {
    if (recvfrom(socket, temp, sizeof(RUDP_Packet) - 1, 0, NULL, 0) == -1) {
      free(temp);
      return -1;
    }
    if (temp->sequalNum == sequal_num && temp->flags.ack) {
      free(temp);
      return 1;
    }
  }
  free(temp);
  return -1;
}


int sending_ack(int socket, RUDP_Packet *rudp) {
    // Create an acknowledgment packet
    RUDP_Packet *ack = malloc(sizeof(RUDP_Packet));
    if (ack == NULL) {
        perror("Failed to allocate memory for acknowledgment packet");
        return -1;
    }
    memset(ack, 0, sizeof(RUDP_Packet));
    ack->flags.ack = 1;
    ack->checksum = calculate_checksum(ack);
    ack->sequalNum = rudp->sequalNum;
    // Send the acknowledgment packet
    if (sendto(socket, ack, sizeof(RUDP_Packet), 0, NULL, 0) == -1) {
        perror("Error: Failed end ack");
        free(ack);
        return -1;
    }
    free(ack);
    return 1;
}
