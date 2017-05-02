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
int sock;
socklen_t slen;
fd_set readFdSet, fullFdSet;
int mode = 0;

int main(int argc, const char * argv[]) {
    
    int signal = NOSIG, state = NOSTATE;
    int waitTimes = 0;
    char buffer[512];
    struct timeval tv;
    tv.tv_sec = TIMEOUT;
    
    slen=sizeof(remotehost);
    
    sock = makeSocket();
    
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
        state = WAITING + SYN;
    }
    else {
        connectTo(SRV_IP);
        printf("Sending SYN!\n");
        sendSignal(SYN);
        state = WAITING + SYN + ACK;
    }
    
    FD_CLR(STDIN_FILENO,&fullFdSet);
    
    while(state != ESTABLISHED){ /* Main connect-loop */
        
        readFdSet = fullFdSet;
        signal = NOSIG;
        
        select(FD_SETSIZE, &readFdSet, NULL, NULL, &tv);
        
        // If the socket is in the read set
        if(FD_ISSET(sock,&readFdSet)){
            signal = readSignal(); //Store messege and remotehost
        }
        
        switch (state) {
                
            case WAITING + SYN:
                if (signal == SYN){
                    printf("\nSending SYNACK!\n");
                    sendSignal(SYN + ACK);
                    waitTimes = 0;
                    state = WAITING + ACK;
                    break;
                }
                else {
                    printf(".");
                    break;
                    
                }
                
            case WAITING + SYN + ACK:
                if (signal == SYN + ACK){
                    printf("Sending ACK!\n");
                    sendSignal(ACK);
                    state = ESTABLISHED;
                    break;
                }
                else{
                    printf("connection timedout...\n");
                    printf("Sending SYN!\n");
                    sendSignal(SYN);
                    break;
                    
                }
                
            case WAITING + ACK:
                waitTimes++;
                if(waitTimes > NWAITS){
                    state = WAITING + SYN;
                    break;
                }
                else if (signal == ACK){
                    state = ESTABLISHED;
                    break;
                }
                else break;
                
            default:
                printf("NO STATE: %d\n",state);
                break;
        }
        
    } /* End select loop */
    
    printf("ESTABLISHED!!!!!!!!\n");
    getchar();
    
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

int sendSignal(int signal){
    
    struct pkt packet;
    
    memset((char *) &packet, 0, sizeof(packet));
    
    packet.flg = signal;
    
    if (sendto(sock, (void*)&packet, sizeof(struct pkt), 0, (struct sockaddr*)&remotehost, slen)==-1)exit(EXIT_FAILURE);
    
    return 1;
}

int readSignal(){
    
    struct pkt packet;
    
    memset((char *) &packet, 0, sizeof(packet));
    
    recvfrom(sock, (void*)&packet, sizeof(struct pkt), 0, (struct sockaddr*)&remotehost,&slen);
    
    return packet.flg;
    
}
