/*Header file for the Reliable UDP (RUDP) API.*/
#ifndef RUDP_API_H
#define RUDP_API_H

#include <stdint.h>   // For uint32_t, uint16_t
#include <stdbool.h>  // For bool type
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <stdbool.h>
#define BUFFER_SIZE 2097152 //Maximum size for sending/receiving data in RUDP.
#define MAX_PACKET_SIZE 8192



// Structure representing the RUDP header
typedef struct
{
    uint16_t length;// Length of the packet (header + data)
    uint16_t seq;// Sequence number of the packet
    uint16_t ack_num;// Acknowledgment number
    uint8_t flags;// Flags indicating the type of packet
    uint32_t checksum;// Checksum of the packet
} RUDP_Header;

// RUDP socket
typedef struct {
int socket_fd;                // UDP socket file descriptor
bool isServer;                // True if the RUDP socket acts like a server, false for client.
bool isConnected;             // True if there is an active connection, false otherwise.
struct sockaddr_in dest_addr; // Destination address. Client fills it when it connects via rudp_connect(), server fills it when it accepts a connection via rudp_accept().
} RUDP_Socket;

// Structure representing an RUDP packet
typedef struct{
    RUDP_Header header; // RUDP header
    char data[MAX_PACKET_SIZE];// Data payload of the packet
} RUDP_Packet;



// Function prototypes:

/**
 * @brief Creates a new RUDP socket.
 * @param isServer Indicates if the socket is acting as a server (true) or client (false).
 * @param listen_port The port number to bind the socket to (for server only).
 * @return A pointer to the newly created RUDP socket structure.
//  */
// RUDP_Socket *rudp_socket(bool isServer, unsigned int port);

// Creating the socket, return -1 if fails
int rudp_socket();

/**
 * @brief Sends data stored in the buffer to the other side.
 * 
 * @param rudp_socket Pointer to the RUDP socket structure.
 * @param flags Flags indicating the type of packet to be sent.
 * @param data Pointer to the data buffer to be sent.
 * @param data_size Size of the data to be sent.
 * @return Number of sent bytes on success, -1 on error.
 */
int rudp_send(RUDP_Socket *rudp_socket, uint8_t flags, char *data, size_t data_size);

/**
 * @brief Receives data from the other side and stores it into the buffer.
 * @param rudp_socket Pointer to the RUDP socket structure.
 * @param packet Pointer to the RUDP packet structure to store the received data.
 * @return Number of received bytes on success, 0 if a FIN packet is received (disconnect), -1 on error.
 */
int rudp_receive(RUDP_Socket *rudp_socket, RUDP_Packet *packet);

/**
 * @brief Closes the RUDP socket and releases all allocated resources. 
 * @param sockfd Pointer to the RUDP socket structure.
 * @return 1 on success, -1 on error.
 */
int rudp_close(RUDP_Socket *sockfd);

//helper methods:

/**
 * @brief Attempts to establish a connection to the specified destination IP and port.
 * @param sockfd Pointer to the RUDP socket structure.
 * @param dest_ip Destination IP address.
 * @param dest_port Destination port number.
 * @return 1 on success, 0 on failure.
 */
int rudp_connect(RUDP_Socket *sockfd, char *dest_ip, unsigned short int dest_port);

/**
 * @brief Disconnects from an actively connected socket.
 * 
 * @param sockfd Pointer to the RUDP socket structure.
 * @return 1 on success, 0 when the socket is already disconnected (failure).
 */
int rudp_disconnect(RUDP_Socket *sockfd);

/**
 * @brief Calculates the checksum for a given data buffer.
 * 
 * @param data Pointer to the data buffer.
 * @param bytes Number of bytes in the data buffer.
 * @return The calculated checksum.
 */
unsigned short int calculate_checksum(void *data, unsigned int bytes);

#endif /* RUDP_API_H */
