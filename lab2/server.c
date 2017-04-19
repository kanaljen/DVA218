/* File: server.c
 * Trying out socket communication between processes using the Internet protocol family.
 */

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/times.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>

#define PORT 5555
#define MAXMSG 512

fd_set activeFdSet; /* Used by select */
int masterSocket;

/* makeSocket
 * Creates and names a socket in the Internet
 * name-space. The socket created exists
 * on the machine from which the function is
 * called. Instead of finding and using the
 * machine's Internet address, the function
 * specifies INADDR_ANY as the host address;
 * the system replaces that with the machine's
 * actual address.
 */

void broadcast(fd_set* exclude, fd_set* include,char* messege);

int makeSocket(unsigned short int port) {
    int sock;
    struct sockaddr_in name;
    
    /* Create a socket. */
    sock = socket(PF_INET, SOCK_STREAM, 0);
    if(sock < 0) {
        perror("Could not create a socket\n");
        exit(EXIT_FAILURE);
    }
    /* Give the socket a name. */
    /* Socket address format set to AF_INET for Internet use. */
    name.sin_family = AF_INET;
    /* Set port number. The function htons converts from host byte order to network byte order.*/
    name.sin_port = htons(port);
    /* Set the Internet address of the host the function is called from. */
    /* The function htonl converts INADDR_ANY from host byte order to network byte order. */
    /* (htonl does the same thing as htons but the former converts a long integer whereas
     * htons converts a short.)
     */
    name.sin_addr.s_addr = htonl(INADDR_ANY);
    /* Assign an address to the socket by calling bind. */
    if(bind(sock, (struct sockaddr *)&name, sizeof(name)) < 0) {
        perror("Could not bind a name to the socket\n");
        exit(EXIT_FAILURE);
    }
    return(sock);
}

void writeMessage(int fileDescriptor, char *message) {
    int nOfBytes;
    
    nOfBytes = (int)write(fileDescriptor, message, strlen(message) + 1);
    if(nOfBytes < 0) {
        perror("writeMessage - Could not write data\n");
        exit(EXIT_FAILURE);
    }
}

/* readMessageFromClient
 * Reads and prints data read from the file (socket
 * denoted by the file descriptor 'fileDescriptor'.
 */
int readMessageFromClient(int fileDescriptor) {
    
    char buffer[MAXMSG],sendBuffer[MAXMSG];
    int nOfBytes;
    fd_set excludeFdSet, includeFdSet;
    
    //Setup includeset
    FD_ZERO(&includeFdSet); //Zero-out the socket set
    includeFdSet = activeFdSet;
    
    //Setup exludeset
    FD_ZERO(&excludeFdSet); //Zero-out the socket set
    FD_SET(masterSocket, &excludeFdSet); //Add mastersocket to exludeset
    FD_SET(fileDescriptor, &excludeFdSet); //Add sender to exludeset
    
    //Read the socket
    nOfBytes = (int)read(fileDescriptor, buffer, MAXMSG);
    if(nOfBytes < 0) {
        perror("Could not read data from client\n");
        return(-1);
    }
    else
        if(nOfBytes == 0)
        /* End of file, connection gracefully closed by client */
            return(-1);
        else{
            /* Data read */
            writeMessage(fileDescriptor,">recived"); //Send ack to sender
            sprintf(sendBuffer,"[%d]: %s",fileDescriptor , buffer); //Add fluff to msg
            printf("%s",sendBuffer); //Print msg in server
            
            /* Brodcast messege to all clients */
            broadcast(&excludeFdSet,&includeFdSet,sendBuffer);
            
        }
    return(0);
}

void broadcast(fd_set* exclude, fd_set* include,char* messege){
    
    for(int i = 0; i < FD_SETSIZE; ++i){
        if(FD_ISSET(i, include)){ // Is in include set
            if (!FD_ISSET(i, exclude)){ //Is NOT in exclude set
                writeMessage(i,messege);
            }
            
        }
        
        
    }
};

int newConnection(void){
    
    socklen_t size;
    int clientSocket;
    struct sockaddr_in clientName;
    char buffer[MAXMSG];
    
    size = sizeof(struct sockaddr_in);
    
    /* Accept the connection request from a client. */
    clientSocket = accept(masterSocket, (struct sockaddr *)&clientName, (socklen_t *)&size);
    if(clientSocket < 0) {
        perror("Could not accept connection\n");
        return -1;
    }
    
    //Print info about connection in server
    printf ("<incoming connection from %s:%d>\n",
             inet_ntoa(clientName.sin_addr),
             ntohs(clientName.sin_port));
    
    //Check to see if blocked adress
    printf("approval pending...\n");
    
    if(strcmp(inet_ntoa(clientName.sin_addr),"127.0.0.2")){ //Changes to filter
        
        FD_SET(clientSocket, &activeFdSet); //Add clientsocket to the set
        
        //Ack the server
        printf("<connection from %s:%d accepted on socket %d>\n",
               inet_ntoa(clientName.sin_addr),
               ntohs(clientName.sin_port), clientSocket);
        
        //Send ack on connection to client
        sprintf (buffer, "[SERVER]: You are connected on socket [%d]",clientSocket);
        writeMessage(clientSocket,buffer);
        
        return clientSocket;
    }
    else{
     
        //Ack the server
        printf("<connection from %s:%d refused>\n",
               inet_ntoa(clientName.sin_addr),
               ntohs(clientName.sin_port));
        
        //Send connect refusal to client
        sprintf (buffer, "[SERVER]: connection from %s:%d refused",
                 inet_ntoa(clientName.sin_addr),
                 ntohs(clientName.sin_port));
        writeMessage(clientSocket,buffer);
        
        close(clientSocket);
        
        return 0;
    }
};

int main(int argc, char *argv[]) {

    int i, clientSocket;
    char sendBuffer[MAXMSG];


    fd_set excludeFdSet,readFdSet;
    
    
    /* Create a socket and set it up to accept connections */
    masterSocket = makeSocket(PORT); //Create the master "listening" socket
    
    /* Listen for connection requests from clients */
    if(listen(masterSocket,1) < 0) {
        perror("Could not listen for connections\n");
        exit(EXIT_FAILURE);
    }
    
    /* Initialize the set of active sockets */
    FD_ZERO(&activeFdSet); //Zero-out the socket set
    FD_SET(masterSocket, &activeFdSet); //Add masterSocket to set
    

    printf("\n[waiting for connections...]\n");
    while(1) {
        /* Block until input arrives on one or more active sockets
         FD_SETSIZE is a constant with value = 1024 */
        readFdSet = activeFdSet;
        if(select(FD_SETSIZE, &readFdSet, NULL, NULL, NULL) < 0) {
            perror("Select failed\n");
            exit(EXIT_FAILURE);
        }
        /* Service all the sockets with input pending */
        for(i = 0; i < FD_SETSIZE; ++i)
            if(FD_ISSET(i, &readFdSet) != 0) {
                if(i == masterSocket) {
                    
                    /* Connection request on master socket */
                    clientSocket = newConnection();
                    
                    //Brodcast connection to other clients
                    sprintf(sendBuffer,"[SERVER]: new client connected on socket [%d]",clientSocket); //Add fluff to msg
                    FD_ZERO(&excludeFdSet); //Zero-out exclude set
                    FD_SET(masterSocket, &excludeFdSet); //Add masterSocket to exclude set
                    FD_SET(clientSocket, &excludeFdSet); //Add new client to exclude set
                    broadcast(&excludeFdSet,&activeFdSet,sendBuffer);
                    
                }
                else {
                    /* Data arriving on an already connected socket */
                    if(readMessageFromClient(i) < 0) {
                        sprintf(sendBuffer,"[SERVER]: client disconnected from socket [%d]",i); //Add fluff to msg
                        printf("%s\n",sendBuffer);
                        close(i); //Close socket
                        FD_CLR(i, &activeFdSet); //Remove socket from set
                        
                        //Broadcast disconnect
                        FD_ZERO(&excludeFdSet); //Zero-out exclude set
                        FD_SET(masterSocket, &excludeFdSet); //Add masterSocket to exclude set
                        broadcast(&excludeFdSet, &activeFdSet, sendBuffer);
                    }
                }
            }
    }
}
