/**
 * @file RUDP_API.h
 * @brief Header file for the Reliable UDP (RUDP) API.
 */

#ifndef RUDP_API_H
#define RUDP_API_H

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdint.h>

#define MAX_PACK_SIZE 4000  /**< Maximum size for data packets. */

/**
 * @struct Flags
 * @brief Struct to represent the flags in RUDP packets.
 */
typedef struct Flags {
  uint8_t fin;        /**< Indicates finishing. */
  uint8_t ack;   /**< Indicates acknowledgment. */
  uint8_t isSyn;    /**< Indicates synchronization. */
  uint8_t isData;      /**< Indicates data packet. */
}Flags;

/**
 * @typedef RUDP_Packet
 * @brief Typedef for RUDP packet structure.
 */
typedef struct _RUDP {
  Flags flags;     /**< Flags for the RUDP packet. */
  uint16_t checksum;           /**< Checksum for the packet. */
  uint16_t length;         /**< Length of data in the packet. */
  int sequalNum;          /**< Sequence number for the packet. */
  char data[MAX_PACK_SIZE];    /**< Data in the packet. */
} RUDP_Packet;

/**
 * @brief Creates a new RUDP socket.
 * @return File descriptor of the created socket, or -1 on failure.
 */
int rudp_socket();

/**
 * @brief Sends data over the RUDP connection.
 * @param socket File descriptor of the RUDP socket.
 * @param data Pointer to the data to be sent.
 * @param size Size of the data to be sent.
 * @return Number of bytes sent on success, or -1 on failure.
 */
int rudp_send(int socket, const char *data, int size);

/**
 * @brief Receives data over the RUDP connection.
 * @param socket File descriptor of the RUDP socket.
 * @param buffer Pointer to the buffer to store received data.
 * @param size Pointer to the variable to store the length of received data.
 * @return 0 on success, or -1 on failure.
 */
int rudp_receive(int socket, char **buffer, int *size);

/**
 * @brief Closes the RUDP socket.
 * @param socket File descriptor of the RUDP socket.
 * @return 0 on success, or -1 on failure.
 */
int rudp_close(int socket);

/**
 * @brief Connects to a remote RUDP socket.
 * @param socket File descriptor of the RUDP socket.
 * @param ip IP address of the remote socket.
 * @param port Port number of the remote socket.
 * @return 1 on success, 0 on failure.
 */
int rudp_connect(int socket, const char *ip,  unsigned short int port);

/**
 * @brief Accepts incoming connection requests on a socket.
 * @param sockfd File descriptor of the listening socket.
 * @param port Port number to bind the socket to.
 * @return 1 on success, 0 on failure.
 */
int rudp_accept(int sockfd,  unsigned short int port);

/**
 * @brief Calculates the checksum for the given RUDP packet.
 * @param rudp Pointer to the RUDP packet for which the checksum is calculated.
 * @return The checksum value.
 */
int calculate_checksum(RUDP_Packet *rudp);

/**
 * @brief Waits for an acknowledgment packet.
 * @param socket File descriptor of the RUDP socket.
 * @param seq_num Expected sequence number of the acknowledgment packet.
 * @param start_time Start time of the waiting period.
 * @param timeout Timeout value for waiting.
 * @return 1 if acknowledgment received, 0 if timeout reached, or -1 on error.
 */
int waiting_ack(int socket, int seq_num, clock_t start_time, clock_t timeout);

/**
 * @brief Sends an acknowledgment packet.
 * @param socket File descriptor of the RUDP socket.
 * @param rudp Pointer to the RUDP packet for which the acknowledgment is sent.
 * @return 1 on success, or -1 on failure.
 */
int sending_ack(int socket, RUDP_Packet *rudp);

#endif 
