#define _POSIX_C_SOURCE 200112L
#include<stdlib.h>
#include<string.h>
#include<stdio.h>
#include<unistd.h>
#include<netdb.h>
#include<sys/socket.h>
#include"Practical.h"
  

void clientSetup(char *serverIP, char *message, char *servPort){


    // Check if the message is too long
    int messageLen = strlen(message);

    if (messageLen > MAXSTRINGLENGTH){
        fprintf(stderr, "%s", "The message is too long\n");
        exit(1);
    }

    

}

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

    // Bind to
    


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