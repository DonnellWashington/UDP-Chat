#define _POSIX_C_SOURCE 200112L
#include<stdlib.h>
#include<string.h>
#include<stdio.h>
#include<unistd.h>
#include<netdb.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include <stdbool.h>

// Handle error with user msg
void DieWithUserMessage(const char *msg, const char *detail);
// Handle error with sys msg
void DieWithSystemMessage(const char *msg);
// Print socket address
void PrintSocketAddress(const struct sockaddr *address, FILE *stream);
// Test socket address equality
bool SockAddrsEqual(const struct sockaddr *addr1, const struct sockaddr *addr2);
// Create, bind, and listen a new TCP server socket
int SetupTCPServerSocket(const char *service);
// Accept a new TCP connection on a server socket
int AcceptTCPConnection(int servSock);
// Handle new TCP client
void HandleTCPClient(int clntSocket);
// Create and connect a new TCP client socket
int SetupTCPClientSocket(const char *server, const char *service);

enum sizeConstants {
  MAXSTRINGLENGTH = 128,
  BUFSIZE = 512,
};

void PrintSocketAddress(const struct sockaddr *address, FILE *stream) {
  // Test for address and stream
  if (address == NULL || stream == NULL)
    return;

  void *numericAddress; // Pointer to binary address
  // Buffer to contain result (IPv6 sufficient to hold IPv4)
  char addrBuffer[INET6_ADDRSTRLEN];
  in_port_t port; // Port to print
  // Set pointer to address based on address family
  switch (address->sa_family) {
  case AF_INET:
    numericAddress = &((struct sockaddr_in *) address)->sin_addr;
    port = ntohs(((struct sockaddr_in *) address)->sin_port);
    break;
  case AF_INET6:
    numericAddress = &((struct sockaddr_in6 *) address)->sin6_addr;
    port = ntohs(((struct sockaddr_in6 *) address)->sin6_port);
    break;
  default:
    fputs("[unknown type]", stream);    // Unhandled type
    return;
  }
  // Convert binary to printable address
  if (inet_ntop(address->sa_family, numericAddress, addrBuffer,
      sizeof(addrBuffer)) == NULL)
    fputs("[invalid address]", stream); // Unable to convert
  else {
    fprintf(stream, "%s", addrBuffer);
    if (port != 0)                // Zero not valid in any socket addr
      fprintf(stream, "-%u", port);
  }
}

bool SockAddrsEqual(const struct sockaddr *addr1, const struct sockaddr *addr2) {
  if (addr1 == NULL || addr2 == NULL)
    return addr1 == addr2;
  else if (addr1->sa_family != addr2->sa_family)
    return false;
  else if (addr1->sa_family == AF_INET) {
    struct sockaddr_in *ipv4Addr1 = (struct sockaddr_in *) addr1;
    struct sockaddr_in *ipv4Addr2 = (struct sockaddr_in *) addr2;
    return ipv4Addr1->sin_addr.s_addr == ipv4Addr2->sin_addr.s_addr
        && ipv4Addr1->sin_port == ipv4Addr2->sin_port;
  } else if (addr1->sa_family == AF_INET6) {
    struct sockaddr_in6 *ipv6Addr1 = (struct sockaddr_in6 *) addr1;
    struct sockaddr_in6 *ipv6Addr2 = (struct sockaddr_in6 *) addr2;
    return memcmp(&ipv6Addr1->sin6_addr, &ipv6Addr2->sin6_addr,
        sizeof(struct in6_addr)) == 0 && ipv6Addr1->sin6_port
        == ipv6Addr2->sin6_port;
  } else
    return false;
}
  
// A function to setup the client
int clientSetup(const char *serverIp, const char *servPort, struct addrinfo **servAddrPtr){

    struct addrinfo addrCriteria;                       // Critera for addreses and matching

    memset(&addrCriteria, 0, sizeof(addrCriteria));     // Zero out struct
    addrCriteria.ai_family = AF_UNSPEC;                 // Address family
    addrCriteria.ai_socktype = SOCK_DGRAM;              // Only datagram sockets
    addrCriteria.ai_protocol = IPPROTO_UDP;             // Only use the UDP protocol

    int rtnVal = getaddrinfo(serverIp, servPort, &addrCriteria, servAddrPtr);

    if (rtnVal != 0) DieWithUserMessage("getaddrinfo() failed", gai_strerror(rtnVal));

    // Create a datagram/UDP socket

    // Socket descriptor
    int sock = socket((*servAddrPtr)->ai_family, (*servAddrPtr)->ai_socktype, (*servAddrPtr)->ai_protocol);

    if (sock < 0) DieWithSystemMessage("sock() failed");

    return sock;

}

// A function to sending messages back and fourth to the server with handling for exiting the program
void clientChat(int sock, struct addrinfo *servAddr, const char *initMessage){
    char sendBuffer[MAXSTRINGLENGTH];
    char revcBuffer[MAXSTRINGLENGTH + 1];

    strncpy(sendBuffer, initMessage, MAXSTRINGLENGTH - 1);

    sendBuffer[MAXSTRINGLENGTH - 1] = '\0';

    // Coversation loop
    while(1){

        // Storing message to send to server
        printf("Client: %s\n", sendBuffer);

        ssize_t numBytes = sendto(sock, sendBuffer, strlen(sendBuffer), 0, servAddr->ai_addr, servAddr->ai_addrlen);

        if (numBytes < 0) DieWithSystemMessage("sendto() failed");

        // If the message is "Goodbye!" break
        if (strstr(sendBuffer, "Goodbye!") != NULL) break;

        struct sockaddr_storage fromAddr;
        socklen_t fromAddrLen = sizeof(fromAddr);
        numBytes = recvfrom(sock, revcBuffer, MAXSTRINGLENGTH, 0, (struct sockaddr *)&fromAddr, &fromAddrLen);

        if (numBytes < 0) DieWithSystemMessage("recvfrom() failed");

        // Message printing, formatting, and null terminating char
        revcBuffer[numBytes] = '\0';

        printf("Server: %s\n", revcBuffer);

        printf("Enter message: ");

        if (fgets(sendBuffer, MAXSTRINGLENGTH,stdin) == NULL) break;

        sendBuffer[strcspn(sendBuffer, "\n")] = '\0';
    
    }

    // The conversation has ended print the address of the communicating party and close socket
    printf("Conversation with ");
    PrintSocketAddress(servAddr->ai_addr, stdout);
    printf(" ended.\n");

}

// A function to setup the server
int serverSetup(char *serverPort){

    struct addrinfo addrCriteria;                               // Criteria for family address

    memset(&addrCriteria, 0, sizeof(addrCriteria));   // Zero out structure

    addrCriteria.ai_family = AF_UNSPEC;                         // Family address
    addrCriteria.ai_flags = AI_PASSIVE;                         // Passively accept any port y any address
    addrCriteria.ai_socktype = SOCK_DGRAM;                      // Only datagram socket
    addrCriteria.ai_protocol = IPPROTO_UDP;                     // Only UDP sockets

    struct addrinfo *servAddr;                                  // List of server addresses

    int rtnVal = getaddrinfo(NULL, serverPort, &addrCriteria, &servAddr);


    if (rtnVal != 0) DieWithUserMessage("getaddrinfo() failed", gai_strerror(rtnVal));

    // Create a socket for incoming connections
    int sock = socket(servAddr->ai_family, servAddr->ai_socktype, servAddr->ai_protocol);

    if (sock < 0) DieWithSystemMessage("socket() failed");

    // Bind to local address
    if (bind(sock, servAddr->ai_addr, servAddr->ai_addrlen) < 0) DieWithSystemMessage("bind() failed");

    // Free addres list allocated by getaddrinfo()
    freeaddrinfo(servAddr);

    return sock;

}

// A function to control sending and receiving between client y server handling for exiting program
void serverChat(int sock){

    char recvBuffer[MAXSTRINGLENGTH + 1];
    char sendBuffer[MAXSTRINGLENGTH];
    struct sockaddr_storage clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);

    // Keep the conversation going until user says goodbye
    while(1){

        // stroring the message received from the user
        ssize_t numBytes = recvfrom(sock, recvBuffer, MAXSTRINGLENGTH, 0, (struct sockaddr *)&clientAddr, &clientAddrLen);

        if (numBytes < 0) DieWithSystemMessage("recvfrom() failed");

        // Printing the message the client sent
        recvBuffer[numBytes] = '\0';
        printf("Client: %s\n", recvBuffer);

        // If client says goodbye the server will print goodbye also and exit
        if (strstr(recvBuffer, "Goodbye!") != NULL){ 
            printf("Server : Goodbye!\n");
            break;
        }

        // Sending reply to the client
        printf("Enter a reply: ");
        if (fgets(sendBuffer, MAXSTRINGLENGTH, stdin) == NULL) break;

        sendBuffer[strcspn(sendBuffer, "\n")] = '\0';

        printf("Server: %s\n", sendBuffer);

        numBytes = sendto(sock, sendBuffer, strlen(sendBuffer), 0, (struct sockaddr *) &clientAddr, clientAddrLen);

        if (numBytes < 0) DieWithSystemMessage("sendto() failed");

    }

    printf("Coversation with ");
    PrintSocketAddress((struct sockaddr *) &clientAddr, stdout);
    printf(" ended.\n");

}

// A function to handle errors and exit program
void DieWithUserMessage(const char *msg, const char *detail){
    fputs(msg, stderr);
    fputs(": ", stderr);
    fputs(detail, stderr);
    fputc('\n', stderr);
    exit(1);
}

// A function to handle errors and exit program
void DieWithSystemMessage(const char *msg){
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[]){

    /* So we need to check for the right command line args first 
       And we know that amount of command line args differ from client to server
       Server needs to be setup first and needs 1 CLA's (port number)
       Client needs 3 (Server address, message, port number)
       So if the cla is 1 then youre the sever and if it has 3 its the client and throw an error otherwsie
    */

    // Server CLA's
    if (argc == 2){
        int sock = serverSetup(argv[1]);
        serverChat(sock);
        close(sock);
    }
    // Client CLA's
    else if (argc == 4){
        struct addrinfo *servAddr;
        int sock = clientSetup(argv[1], argv[3], &servAddr);
        clientChat(sock, servAddr,argv[2]);
        freeaddrinfo(servAddr);
        close(sock);
    }
    
    else DieWithUserMessage("Parameter(s)", "if server <Server Port> if client <Server Address> <Echo Word> <Server Port>");
    
    return 0;

}

