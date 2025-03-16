#define _POSIX_C_SOURCE 200112L
#include<stdlib.h>
#include<string.h>
#include<stdio.h>
#include<unistd.h>
#include<netdb.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include"Practical.h"

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
void clientSetup(char *serverIP, char *message, char *servPort){


    // Check if the message is too long
    int messageLen = strlen(message);

    if (messageLen > MAXSTRINGLENGTH){
        fprintf(stderr, "%s", "The message is too long\n");
        exit(1);
    }

    struct addrinfo addrCriteria;                       // Critera for addreses and matching

    memset(&addrCriteria, 0, sizeof(addrCriteria));     // Zero out struct
    addrCriteria.ai_family = AF_UNSPEC;                 // Address family
    addrCriteria.ai_socktype = SOCK_DGRAM;              // Only datagram sockets
    addrCriteria.ai_protocol = IPPROTO_UDP;             // Only use the UDP protocol

    struct addrinfo *servAddr;                          // List of server address

    int rtnVal = getaddrinfo(serverIP, servPort, &addrCriteria, &servAddr);

    if (rtnVal != 0) DieWithUserMessage("getaddrinfo() failed", gai_strerror(rtnVal));

    // Create a datagram/UDP socket

    // Socket descriptor
    int sock = socket(servAddr->ai_family, servAddr->ai_socktype, servAddr->ai_protocol);

    if (sock < 0) DieWithSystemMessage("sock() failed");

    // Send message to the server
    ssize_t numBytes = sendto(sock, message, messageLen, 0, servAddr->ai_addr, servAddr->ai_addrlen);

    if (numBytes < 0) DieWithSystemMessage("send to() failed");
    else if (numBytes != messageLen) DieWithUserMessage("sendto() error", "sent unexpected number of bytes");

    // Response received

    struct sockaddr_storage fromAddr;

    // Set length of address struct
    socklen_t fromAddrLen = sizeof(fromAddr);

    char buffer[MAXSTRINGLENGTH + 1];
    numBytes = recvfrom(sock, buffer, MAXSTRINGLENGTH, 0, (struct sockaddr *) &fromAddr, &fromAddrLen);

    if (numBytes < 0) DieWithSystemMessage("recvfrom() failed");
    else if (numBytes != messageLen) DieWithUserMessage("recvfrom() error", "received unexpected number of bytes");
    
    // Make sure the packet was received from the expected source
    if (!SockAddrsEqual(servAddr->ai_addr, (struct sockaddr *) &fromAddr))
    DieWithUserMessage("recvfrom()", "received a packet from unknow soucre");

    // Give memory allocated back to the OS
    freeaddrinfo(servAddr);            

    buffer[messageLen] = '\0';

    printf("Received: %s\n", buffer);

    close(sock);
    

}

// A function to setup the server
void serverSetup(char *serverPort){

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
    if (bind(sock, servAddr->ai_addr, servAddr->ai_addrlen) < 0){
        DieWithSystemMessage("socket() failed");
    }

    // Free addres list allocated by getaddrinfo()
    freeaddrinfo(servAddr);

    for ( ;; ){
        struct sockaddr_storage clntAddr;                       // Client address sets length to client address struct
        socklen_t clntAddrLen = sizeof(clntAddr);

        // Block until message received from client
        char buffer[MAXSTRINGLENGTH];
        ssize_t numBytesRecvd = recvfrom(sock, buffer, MAXSTRINGLENGTH, 0, (struct sockaddr *) &clntAddr, &clntAddrLen);

        if (numBytesRecvd < 0) DieWithSystemMessage("recvfrom() failed");

        fputs("Handling client client ", stdout);
        PrintSocketAddress((struct sockaddr *) &clntAddr, stdout);
        fputc('\n', stdout);

        // Send received datagram back to the client
        ssize_t numBytesSent = sendto(sock, buffer,numBytesRecvd, 0, (struct sockaddr *) &clntAddr, sizeof(clntAddr));

        if (numBytesSent < 0) DieWithSystemMessage("sendto() failed");

        else if (numBytesSent != numBytesRecvd) DieWithUserMessage("sendto()", "sent unexpected number of bytes");
        

    }
    
    
    


}

void DieWithUserMessage(const char *msg, const char *detail){
    fputs(msg, stderr);
    fputs(": ", stderr);
    fputs(detail, stderr);
    fputc('\n', stderr);
    exit(1);
}

void DieWithSystemMessage(const char *msg){
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[]){

    /* So we need to check for the right command line args first 
       And we know that amount of command line args differ from client to server
       Server needs to be setup first and needs 1 CLA's (port number)
       Client needs 3 (Server address, message, port number)
       So if the cla is 1 then youre the sever and if it has 3 its the client and throw and error otherwsie
    */

    char *server = argv[1];
    char *echoMessage = argv[2];
    char *servPort = argv[3];

    // Server CLA's
    if(argc == 1) serverSetup(servPort);

    // Client CLA's
    if (argc == 3) clientSetup(server, echoMessage, servPort);

    else DieWithUserMessage("Parameter(s)", "if server <Server Port> if client <Server Address> <Echo Word> <Server Port>");
    

    return 0;

}
