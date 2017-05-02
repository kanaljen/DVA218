//
//  main.c
//  DVA218 - lab3
//
//  Created by Kanaljen on 2017-05-01.
//  Copyright © 2017 Kanaljen. All rights reserved.
//

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/time.h>
#include "main.h"

struct sockaddr_in localhost, remotehost;
int sock,socktemp;
socklen_t slen;
fd_set readFdSet, fullFdSet;
int mode = 0;

int main(int argc, const char * argv[]) {
    
    int signal = NOSIG, i;
    int stateDatabase[FD_SETSIZE];
    int waitTimes[FD_SETSIZE];
    char buffer[512];
    struct timeval tv;
    tv.tv_sec = TIMEOUT;
    int clientSock;
    
    slen=sizeof(remotehost);
    
    sock = makeSocket();
    
    printf("sock: %d\nsocktemp: %d\n",sock,socktemp);
    
    // FD stuff
    FD_ZERO(&fullFdSet);
    FD_SET(STDIN_FILENO,&fullFdSet);
    
    
    // Select mode for process
    
    printf("Select Mode: 1 for server, 2 for client:");
    
    while((mode != SERVER)&&(mode != CLIENT)){
        
        printf("");
        select(FD_SETSIZE, &fullFdSet, NULL, NULL, NULL);
        fgets(buffer, sizeof(buffer), stdin);
        size_t ln = strlen(buffer)-1;
        if (buffer[ln] == '\n') buffer[ln] = '\0';
        mode = atoi(buffer);
        if(mode != SERVER && mode != CLIENT)printf("> invalid mode\n");
        
    }
    
    if(mode == SERVER){
        bindSocket(sock);
        stateDatabase[sock] = WAITING + SYN;
    }
    else if(mode == CLIENT){
        connectTo(SRV_IP);
        printf("[%d]Sending SYN!\n",sock);
        sendSignal(sock,SYN);
        stateDatabase[sock] = WAITING + SYN + ACK;
    }
    
    FD_CLR(STDIN_FILENO,&fullFdSet);
    
    while(1){ /* Main connect-loop */
        
        readFdSet = fullFdSet;
        signal = NOSIG;
        
        select(FD_SETSIZE, &readFdSet, NULL, NULL, &tv);
        
        // If the socket is in the read set
        for(i = sock;i < FD_SETSIZE;i++){
            if(FD_ISSET(i,&fullFdSet)){
                
                if(FD_ISSET(i,&readFdSet)){
                    
                    signal = readSignal(i); //Store messege and remotehost
                    
                }
                
                
                switch (stateDatabase[i]) {
                        
                    case WAITING + SYN:
                        if (signal == SYN){
                            
                            clientSock = makeSocket();
                            printf("new client [%d]\n",clientSock);
                            printf("[%d] Sending SYNACK!\n",clientSock);
                            sendSignal(clientSock,SYN + ACK);
                            FD_SET(clientSock,&fullFdSet);
                            waitTimes[clientSock] = 0;
                            stateDatabase[clientSock] = WAITING + ACK;
                            break;
                        }
                        else {
                            printf("[%d] waiting\n",i);
                            break;
                            
                        }
                        
                    case WAITING + SYN + ACK:
                        if (signal == SYN + ACK){
                            printf("[%d] Sending ACK!\n",i);
                            sendSignal(i,ACK);
                            stateDatabase[i] = ESTABLISHED;
                            printf("[%d] ESTABLISHED",i);
                            break;
                        }
                        else{
                            printf("connection timedout...\n");
                            printf("[%d]Sending SYN!\n",i);
                            sendSignal(i,SYN);
                            break;
                            
                        }
                        
                    case WAITING + ACK:
                        waitTimes[i]++;
                        if(waitTimes[i] >= NWAITS){
                            printf("[%d] connection from client lost\n",i);
                            close(i);
                            FD_CLR(i,&fullFdSet);
                            break;
                        }
                        else if (signal == ACK){
                            printf("[%d] ESTABLISHED\n",i);
                            stateDatabase[i] = ESTABLISHED;
                            
                            break;
                        }
                        else{
                            printf("[%d] Sending new SYNACK!\n",i);
                            sendSignal(i, SYN + ACK);
                            break;
                        }
                        
                    case ESTABLISHED:
                        FD_SET(STDIN_FILENO,&fullFdSet);
                        printf("[%d] *\n",i);
                        break;
                        
                    default:
                        printf("NO STATE: %d\n",stateDatabase[i]);
                        break;
                }
            }
        }
    } /* End select loop */
    
    return 0;
}

int makeSocket(void){
    
    int sock;
    
    // Create the UDP socket
    if ((sock=socket(AF_INET, SOCK_DGRAM, 0)) < 0)exit(EXIT_FAILURE);
    
    return sock;
}

int connectTo(char* server){
    
    memset((char *) &remotehost, 0, sizeof(remotehost));
    
    remotehost.sin_family = AF_INET;
    
    remotehost.sin_port = htons(PORT);
    
    if (inet_aton(server, &remotehost.sin_addr)==0) {
        fprintf(stderr, "inet_aton() failed\n");
        exit(EXIT_FAILURE);
    }
    
    FD_SET(sock,&fullFdSet);
    
    return 1;
}

int bindSocket(int socket){
    
    memset((char *) &localhost, 0, sizeof(localhost));
    
    localhost.sin_family = AF_INET;
    
    localhost.sin_port = htons(PORT);
    
    localhost.sin_addr.s_addr = htonl(INADDR_ANY);
    
    if (bind(socket, (struct sockaddr*)&localhost, sizeof(localhost))==-1)exit(EXIT_FAILURE);
    
    FD_SET(sock,&fullFdSet);
    
    return 1;
}

int sendSignal(int socket,int signal){
    
    struct pkt packet;
    
    memset((char *) &packet, 0, sizeof(packet));
    
    packet.flg = signal;
    
    sleep(SENDDELAY);
    
    if(mode == SERVER){
        if (sendto(socket, (void*)&packet, sizeof(struct pkt), 0, (struct sockaddr*)&remotehost, slen)==-1)exit(EXIT_FAILURE);
    }
    else{
        if (sendto(sock, (void*)&packet, sizeof(struct pkt), 0, (struct sockaddr*)&remotehost, slen)==-1)exit(EXIT_FAILURE);
    }
    
    
    return 1;
}

int readSignal(int socket){
    
    struct pkt packet;
    
    memset((char *) &packet, 0, sizeof(packet));
    
    recvfrom(socket, (void*)&packet, sizeof(struct pkt), 0, (struct sockaddr*)&remotehost,&slen);
    
    return packet.flg;
    
}
