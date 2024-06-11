# Project Documentation: Building a Simple QUIC Protocol in Python

## Introduction
In this project, we implemented a simplified version of the QUIC (Quick UDP Internet Connection) protocol in Python. QUIC is an emerging transport protocol developed by Google that aims to improve web performance by reducing latency and improving security. Our project focuses on building a basic QUIC client and server to demonstrate key features of the protocol.

## Overview
The project consists of three main components:
1. *QUIC Client*: Responsible for initiating connections to the server, sending requests, and receiving responses.
2. *QUIC Server*: Listens for incoming connections from clients, processes requests, and sends back responses.
3. *Packet Handling*: Defines classes and functions for encoding and decoding QUIC packets, managing sockets, and handling stream payloads.

## Implementation Details
### QUIC Client
- Establishes a connection to the server using QUIC.
- Sends requests for data over multiple streams.
- Receives responses from the server and handles stream data.

### QUIC Server
- Listens for incoming connections from clients.
- Accepts requests and processes them, generating random data for streams.
- Sends back responses containing stream data.

### Packet Handling
- Defines classes for encoding and decoding QUIC packets.
- Manages sockets for communication between client and server.
- Handles stream payloads for transferring data over QUIC.

## Unit Testing
We conducted unit testing to ensure the correctness and reliability of our implementation. Test cases were designed to cover various scenarios, including packet encoding and decoding, socket functionality, and stream payload handling.

## Conclusion
By building a simplified QUIC protocol in Python, we gained a deeper understanding of network protocols and transport layer technologies. This project demonstrates the fundamental concepts of QUIC and provides a solid foundation for further exploration and development in this area.
