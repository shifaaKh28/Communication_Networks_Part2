/* Header file for the Reliable UDP (RUDP) API. */

#ifndef RUDP_API_H
#define RUDP_API_H

#include <stdint.h>  
#include <stdbool.h>  
#include <arpa/inet.h>
#include <netinet/tcp.h>

// Maximum size for data payload in an RUDP packet
#define MAX_SIZE 60000 

// Structure to represent various flags in the RUDP protocol
struct Flags {
    bool fin;        // Indicates if the packet is a finish flag
    bool ack;               // Indicates if the packet is an acknowledgment
    bool isSyn;    // Indicates if the packet is synchronized
    bool isData;      // Indicates if the packet contains data
};

// Structure representing a Reliable UDP packet
typedef struct RUDP_Packet {
    struct Flags flags; // Flags indicating the type of packet
    int checksum;       // Checksum of the packet
    int sequalNum;      // Sequence number of the packet
    int length;     // Length of the data payload
    char data[MAX_SIZE]; // Data payload
} RUDP_Packet;


/**
 * @brief Creates a new RUDP socket.
 * @return File descriptor of the created socket, or -1 on failure.
 */
int rudp_socket();

/**
 * @brief Waits for an incoming connection request on the specified RUDP socket.
 * @param socket The file descriptor of the RUDP socket.
 * @param port The port number to listen on for incoming connections.
 * @return 1 on successful connection, -1 on failure.
 */
int rudp_accept(int socket, int port);

/**
 * @brief Sends data over the RUDP connection.
 * @param sockfd File descriptor of the RUDP socket.
 * @param data Pointer to the data to be sent.
 * @param size Size of the data to be sent.
 * @return Number of bytes sent on success, or -1 on failure.
 */
int rudp_send(int sockfd, const char *data, int size);

/**
 * @brief Receives data over the RUDP connection.
 * @param sockfd File descriptor of the RUDP socket.
 * @param buffer Pointer to a pointer that will store the received data.
 * @param length Pointer to an integer that will store the length of the received data.
 * @return 0 on success, -1 on failure.
 */
int rudp_receive(int sockfd, char **buffer, int *size);

/**
 * @brief Closes the RUDP socket.
 * @param sockfd File descriptor of the RUDP socket.
 * @return 0 on success, -1 on failure.
 */
int rudp_close(int sockfd);

/**
 * @brief Establishes a connection to the specified IP address and port.
 * @param socket File descriptor of the RUDP socket.
 * @param ip IP address of the destination.
 * @param port Port number of the destination.
 * @return 0 on success, -1 on failure.
 */
int rudp_connect(int socket, const char *ip, unsigned short int port);

/**
* @brief Calculates the checksum for the given RUDP packet.
*@param rudp Pointer to the RUDP packet for which the checksum is calculated.
*@return The checksum value as an unsigned short integer.
*/
unsigned short int calculate_checksum(RUDP_Packet *rudp);

/**
 * @brief Waits for acknowledgment from the receiver.
 * @param socket File descriptor of the RUDP socket.
 * @param sequal_num Sequence number of the packet to wait for acknowledgment.
 * @param st Start time for timeout calculation.
 * @param tout Timeout value in clock ticks.
 * @return 0 on success (acknowledgment received), -1 on failure (timeout).
 */
int wait_ack(int sockfd, int sequal_num, clock_t s, clock_t t);

/**
 * @brief Sends an acknowledgment packet for the received data packet.
 * @param socket File descriptor of the RUDP socket.
 * @param rudp Pointer to the received RUDP packet.
 * @return 0 on success, -1 on failure.
 */
int send_ack(int sockfd, RUDP_Packet *rudp);

/**
 * @brief Sets the timeout value for the RUDP socket.
 * @param socket File descriptor of the RUDP socket.
 * @param time Timeout value in seconds.
 * @return 0 on success, -1 on failure.
 */
int set_time(int sockfd, int time);


#endif /* RUDP_API_H */
