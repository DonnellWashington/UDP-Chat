#include<stdlib.h>
#include<string.h>
#include<stdio.h>
#include<unistd.h>
#include<netdb.h>
#include<sys/socket.h>
#include"Practical.h"

// void clientSetup(char *serverIP, char *message, char *servPort){


//     // Check if the message is too long
//     int messageLen = strlen(message);

//     if (messageLen > MAXSTRINGLENGTH) DieWithUserMessage(messageLen, "Steing too long");

    

// }

// void serverSetup(char *serverPort){

//     printf("hELLOW wOLRD");

// }

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

    // char *serverIP = &server;
    // char *message = &echoMessage;
    // char *serverPort = &serverPort;

    // // Server CLA's
    // if(argc == 1){
    //     serverSetup(*serverPort);
    // }

    // // Client CLA's
    // if (argc == 3){
    //     clientSetup(*serverIP, *message, *serverPort);
    // }

    // else DieWithUserMessage("Parameter(s)", "if server <Server Port> if client <Server Address> <Echo Word> <Server Port>");
    

    return 0;

}