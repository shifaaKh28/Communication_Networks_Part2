
#include <stdio.h>     // Standard input-output functions
#include <stdlib.h>    // Standard library functions
#include <string.h>    // String manipulation functions
#include <unistd.h>    // POSIX operating system API
#include <sys/socket.h> // Socket programming functions and structures
#include <arpa/inet.h>  // Definitions for internet operations
#include <netinet/tcp.h> // Definitions for TCP/IP protocol
#include <stdbool.h>     // Standard boolean type and values
#include <sys/time.h>    // Time-related functions and structures
#include "RUDP.API.h"

#define MAX_PACKET_SIZE 8192
#define BUFFER_SIZE 2097152
#define SERVER_IP "127.0.0.1"
#define MAX_WAIT_TIME 3

/**
 * @brief Enum representing flags used in the RUDP protocol.
 * These flags indicate the type of packet or control message in the RUDP protocol.
 */
typedef enum {
    SYN = 1,       /**< Synchronization flag */
    ACK = 2,       /**< Acknowledgment flag */
    SYN_ACK = 3,   /**< SYN-ACK flag */
    FIN = 4,       /**< Finish flag */
    FIN_ACK = 6,   /**< FIN-ACK flag */
    PUSH = 16      /**< Push flag */
} RUDP_Flag;

// RUDP_Socket *rudp_socket(bool isServer, unsigned int port)
// {
//     // Allocate memory for the socket structure
//     RUDP_Socket *sockfd = malloc(sizeof(RUDP_Socket));
//     if (!sockfd) {
//         perror("malloc(3)");
//         return NULL;
//     }

//     // Create a sockaddr_in structure for the server address
//     struct sockaddr_in server = {0};
//     server.sin_family = AF_INET;

//     // Set the server's IP address to localhost
//     if (inet_pton(AF_INET, SERVER_IP, &server.sin_addr) <= 0) {
//         perror("inet_pton(3)");
//         free(sockfd);
//         return NULL;
//     }

//     // Set the server's port to the specified port (network byte order)
//     server.sin_port = htons(port);

//     // Create the UDP socket
//     sockfd->socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
//     if (sockfd->socket_fd < 0) {
//         perror("socket(2)");
//         free(sockfd);
//         return NULL;
//     }

//     sockfd->isServer = isServer;
//     sockfd->isConnected = false;

//     // Bind the socket if it's a server
//     if (isServer) {
//         if (bind(sockfd->socket_fd, (struct sockaddr *)&server, sizeof(server)) < 0) {
//             perror("bind(2)");
//             close(sockfd->socket_fd);
//             free(sockfd);
//             return NULL;
//         }
//     }
//     else {
//         // Set the receive timeout for client sockets
//         struct timeval tv = {MAX_WAIT_TIME, 0};
//         if (setsockopt(sockfd->socket_fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv)) == -1) {
//             perror("setsockopt(2)");
//             close(sockfd->socket_fd);
//             free(sockfd);
//             return NULL;
//         }
//     }

//     return sockfd;
// }

RUDP_Socket *rudp_socket() {
    // Allocate memory for the socket structure
    RUDP_Socket *sockfd = malloc(sizeof(RUDP_Socket));
    if (!sockfd) {
        perror("malloc(3)");
        return NULL;
    }

    // Create the UDP socket
    sockfd->socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd->socket_fd < 0) {
        perror("socket(2)");
        free(sockfd);
        return NULL;
    }

    sockfd->isConnected = false;

    return sockfd;
}

unsigned short int calculate_checksum(void *data, unsigned int bytes) {
    unsigned short int *buffer = (unsigned short int *)data;
    unsigned int sum = 0;
    
    // Sum all 16-bit words in the buffer
    while (bytes > 1) {
        sum += *buffer++;
        bytes -= 2;
    }
    
    // Add any remaining byte (if the buffer length is odd)
    if (bytes > 0) {
        sum += *((unsigned char *)buffer);
    }
    
    // Fold 32-bit sum to 16 bits
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    
    // Take the one's complement
    return ~sum;
}

int rudp_receive(RUDP_Socket *sockfd, RUDP_Packet *packet) {
    // Check if the socket is connected
    if (!sockfd->isConnected) {
        fprintf(stderr, "Error: Socket is not connected.\n");
        return -1;
    }

    // Receive data from the socket
    ssize_t recv_bytes = recvfrom(sockfd->socket_fd, packet, sizeof(RUDP_Packet), 0, NULL, NULL);
    if (recv_bytes == -1) {
        perror("recvfrom failed");
        return -1;
    }

    // Extract the flags from the received packet
    uint8_t received_flags = packet->header.flags;

    // Check if the received packet is a control packet (SYN, SYN-ACK, ACK, FIN, FIN-ACK)
    if (received_flags == SYN || received_flags == SYN_ACK || received_flags == ACK || received_flags == FIN_ACK) {
        // Return the number of received bytes without processing the data payload
        return recv_bytes;
    }

    // Check if the received packet is a FIN packet
    if (received_flags == FIN) {
        printf("Received FIN packet. Disconnecting...\n");
        // Update socket status to disconnected
        sockfd->isConnected = false;
        return 0; // Indicate disconnection by returning 0
    }

    // If the packet is not a control packet, process the data payload
    size_t data_size = recv_bytes - sizeof(RUDP_Header);

    // Check for data corruption using checksum if applicable
    if (received_flags == PUSH) {
        // Calculate checksum and verify
        unsigned short int checksum = calculate_checksum(packet->data, data_size);
        if (checksum != packet->header.checksum) {
            printf("Checksum failed for sequence number %d: %d\n", packet->header.seq, checksum);
            // Optionally handle checksum failure
            // For now, just return and ignore the packet
            return 0;
        }
    }

    // Data received successfully
    printf("Received %zd bytes of data.\n", data_size);
    return data_size;
}

#include "RUDP.API.h"
int rudp_connect(RUDP_Socket *sockfd, char *dest_ip, unsigned short int dest_port) {
    // Check if the socket is already connected or acting as a server
    if (sockfd->isServer || sockfd->isConnected) {
        fprintf(stderr, "Error: Socket is already connected or acting as a server.\n");
        return 0;
    }

    // Create a sockaddr_in structure for the destination address
    memset(&sockfd->dest_addr, 0, sizeof(sockfd->dest_addr));
    sockfd->dest_addr.sin_family = AF_INET;

    // Convert destination IP to network format and store it in dest_addr
    if (inet_pton(AF_INET, dest_ip, &sockfd->dest_addr.sin_addr) <= 0) {
        perror("inet_pton(3)");
        return 0;
    }
    sockfd->dest_addr.sin_port = htons(dest_port);

    // Send SYN packet
    printf("Sending SYN packet to %s:%u...\n", dest_ip, dest_port);
    int sent = rudp_send(sockfd, SYN, NULL, 0);
    if (sent == -1) {
        printf("Error: Failed to send SYN packet.\n");
        return 0;
    }

    // Receive SYN-ACK packet
    RUDP_Packet packet;
    int recv = rudp_receive(sockfd, &packet);
    if (recv == -1) {
        printf("Error: Failed to receive SYN-ACK packet.\n");
        return 0;
    }

    // Check if received packet is SYN-ACK
    if (packet.header.flags == SYN_ACK) {
        printf("Received SYN-ACK packet.\n");

        // Send ACK packet
        printf("Sending ACK packet...\n");
        sent = rudp_send(sockfd, ACK, NULL, 0);
        if (sent == -1) {
            printf("Error: Failed to send ACK packet.\n");
            return 0;
        }
        
        // Update socket status to connected
        sockfd->isConnected = true;

        // Connection successful
        printf("Connection established with %s:%u.\n", dest_ip, dest_port);
        return 1;
    } else {
        printf("Error: Received unexpected packet (flag: %d).\n", packet.header.flags);
        return 0;
    }
}

int rudp_disconnect(RUDP_Socket *sockfd) {
    // Check if the socket is not connected
    if (!sockfd->isConnected) {
        fprintf(stderr, "Error: Socket is not connected.\n");
        return 0;
    }

    // Check if the socket is a client socket
    if (!sockfd->isServer) {
        // Send FIN packet
        printf("Sending FIN packet...\n");
        int sent = rudp_send(sockfd, FIN, NULL, 0);
        if (sent == -1) {
            printf("Error: Failed to send FIN packet.\n");
            return 0;
        }

        // Receive FIN-ACK packet
        RUDP_Packet packet;
        int recv = rudp_receive(sockfd, &packet);
        if (recv == -1) {
            printf("Error: Failed to receive FIN-ACK packet.\n");
            return 0;
        }

        // Check if received packet is FIN-ACK
        if (packet.header.flags == FIN_ACK) {
            printf("Received FIN-ACK packet.\n");

            // Send ACK packet
            printf("Sending ACK packet.\n");
            sent = rudp_send(sockfd, ACK, NULL, 0);
            if (sent == -1) {
                printf("Error: Failed to send ACK packet.\n");
                return 0;
            }

            // Update socket status to disconnected
            sockfd->isConnected = false;

            // Connection closed successfully
            printf("Connection closed.\n");
            return 1;
        } else {
            printf("Error: Received unexpected packet (flag: %d).\n", packet.header.flags);
            return 0;
        }
    } else {
        fprintf(stderr, "Error: Disconnect operation is only supported on client sockets.\n");
        return 0;
    }
}

int rudp_close(RUDP_Socket *sockfd) {
    // Check if the socket is already closed
    if (sockfd == NULL) {
        fprintf(stderr, "Error: Socket is already closed.\n");
        return 0;
    }

    // Close the socket file descriptor
    close(sockfd->socket_fd);

    // Free the dynamically allocated memory for the socket structure
    free(sockfd);

    // Set the socket pointer to NULL to indicate closure
    sockfd = NULL;

    // Socket closed successfully
    printf("Socket closed.\n");
    return 1;
}

int rudp_accept(RUDP_Socket *server_socket) {
    // Check if the socket is already connected
    if (server_socket->isConnected) {
        fprintf(stderr, "Error: Socket is already connected.\n");
        return -1;
    }

    // Check if the socket is not a server socket
    if (!server_socket->isServer) {
        fprintf(stderr, "Error: Socket is not a server socket.\n");
        return -1;
    }

    // Receive SYN packet from the client
    RUDP_Packet syn_packet;
    ssize_t recv_bytes = rudp_receive(server_socket, &syn_packet);
    if (recv_bytes < 0) {
        fprintf(stderr, "Error receiving SYN packet.\n");
        return -1;
    }

    // Check if the received packet is a SYN packet
    if (syn_packet.header.flags != SYN) {
        fprintf(stderr, "Error: Received packet is not a SYN packet.\n");
        return -1;
    }

    // Create and send SYN-ACK packet to the client
    RUDP_Packet syn_ack_packet;
    syn_ack_packet.header.flags = SYN_ACK;
    ssize_t sent_bytes = rudp_send(server_socket, SYN_ACK, (char*)&syn_ack_packet, sizeof(RUDP_Header));
    if (sent_bytes < 0) {
        fprintf(stderr, "Error sending SYN-ACK packet.\n");
        return -1;
    }

    // Receive ACK packet from the client
    RUDP_Packet ack_packet;
    recv_bytes = rudp_receive(server_socket, &ack_packet);
    if (recv_bytes < 0) {
        fprintf(stderr, "Error receiving ACK packet.\n");
        return -1;
    }

    // Check if the received packet is an ACK packet
    if (ack_packet.header.flags != ACK) {
        fprintf(stderr, "Error: Received packet is not an ACK packet.\n");
        return -1;
    }

    // Connection established successfully
    printf("Connection established with client.\n");
    server_socket->isConnected = true;

    return 0;
}

int rudp_send(RUDP_Socket *rudp_socket, uint8_t flags, char *data, size_t data_size)
{
    // Ensure that the socket and data are valid
    if (rudp_socket == NULL || data == NULL || data_size == 0) {
        return -1; // Invalid arguments
    }

    // Calculate the number of packets needed to send the data
    size_t num_packets = (data_size + MAX_PACKET_SIZE - 1) / MAX_PACKET_SIZE;

    // Send each packet individually
    for (size_t i = 0; i < num_packets; ++i) {
        // Calculate the size of the current packet's data
        size_t packet_data_size = (i == num_packets - 1) ? (data_size % MAX_PACKET_SIZE) : MAX_PACKET_SIZE;

        // Create a packet structure
        RUDP_Packet packet;
        packet.header.flags = flags;
        packet.header.seq = i; // Incremental sequence number for each packet
        packet.header.length = sizeof(RUDP_Header) + packet_data_size;

        // Copy the data into the packet
        memcpy(packet.data, data + i * MAX_PACKET_SIZE, packet_data_size);

        // Calculate and set the checksum
        packet.header.checksum = calculate_checksum(packet.data, packet_data_size);

        // Send the packet
        if (sendto(rudp_socket->socket_fd, (const char *)&packet, sizeof(RUDP_Header) + packet_data_size, 0,
                   (struct sockaddr *)&rudp_socket->dest_addr, sizeof(rudp_socket->dest_addr)) == -1) {
            return -1; // Error occurred during send
        }
    }

    return data_size; // Return the size of the data sent
}

