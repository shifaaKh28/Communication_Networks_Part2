
# Reliable UDP (RUDP) Protocol - Part 2

This project implements a Reliable UDP (RUDP) protocol, which ensures reliable data transmission over the inherently unreliable UDP protocol. The implementation includes a sender and receiver, each designed to handle data transmission with added reliability features like acknowledgments and retransmissions.

## Files

- **RUDP_Sender.c**: 
  - This file contains the implementation of the RUDP sender program. The sender transmits data over a UDP connection and waits for acknowledgments from the receiver. If an acknowledgment is not received within a specified time, the sender retransmits the data.
  
- **RUDP_Receiver.c**: 
  - This file contains the implementation of the RUDP receiver program. The receiver listens for incoming data packets, sends back acknowledgments, and ensures that all data is received correctly and in order.
  
- **RUDP_API.h**: 
  - This header file contains the function prototypes and definitions necessary for the RUDP protocol. It provides the interface for creating sockets, sending and receiving data, and managing connections using RUDP.
  
- **Makefile**: 
  - The makefile is used to compile the RUDP sender and receiver programs. It defines the necessary build rules and dependencies.

## How It Works

### Sender (`RUDP_Sender.c`)

1. The sender program starts by parsing command-line arguments to extract the IP address and port number of the receiver.
2. It generates a random 2MB data buffer to be sent.
3. The sender creates a socket and attempts to establish a connection with the receiver.
4. Once connected, the sender transmits the data in packets.
5. For each packet sent, the sender waits for an acknowledgment from the receiver. If an acknowledgment is not received within a timeout period, the sender retransmits the packet.
6. The user is prompted to send the data again or exit the program.
7. After all data is sent, the connection is closed, and the program exits.

### Receiver (`RUDP_Receiver.c`)

1. The receiver program starts by parsing the command-line arguments to extract the port number on which to listen for incoming connections.
2. The receiver creates a socket and waits for an incoming connection from the sender.
3. Upon establishing a connection, the receiver begins receiving data packets.
4. The received data is written to a file, and acknowledgments are sent back to the sender for each packet received.
5. The receiver calculates and logs the time taken and the speed of the data transfer for each run.
6. After receiving all data, the connection is closed, and the program prints out the statistics of the transfer.

## Compilation

To compile the RUDP sender and receiver programs, use the provided Makefile:

```bash
make
```

This command will generate two executable files: `RUDP_Sender` and `RUDP_Receiver`.

## Usage

### Running the Receiver

To start the receiver:

```bash
./RUDP_Receiver -p <port>
```

- `<port>`: The port number on which the receiver will listen for incoming connections.

### Running the Sender

To start the sender:

```bash
./RUDP_Sender -ip <receiver_ip> -p <port>
```

- `<receiver_ip>`: The IP address of the receiver.
- `<port>`: The port number on which the receiver is listening.

### Example

1. Start the receiver:

```bash
./RUDP_Receiver -p 1234
```

2. Start the sender:

```bash
./RUDP_Sender -ip 127.0.0.1 -p 1234
```

## Notes

- The RUDP implementation adds reliability to UDP by implementing mechanisms like retransmissions and acknowledgments.
- The sender prompts the user to resend the data or exit after each transfer.
- The receiver logs the transfer statistics, including the time taken and the speed of the transfer.

