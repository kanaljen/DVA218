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

struct sockaddr_in localhost, remotehost[FD_SETSIZE];
int sock;
socklen_t slen;
fd_set readFdSet, fullFdSet;
int mode = 0;
struct serie *head = NULL;

int main(int argc, const char * argv[]) {
    
    int signal = NOSIG, i;
    int stateDatabase[FD_SETSIZE];
    int waitTimes[FD_SETSIZE];
    
    char buffer[512];
    
    struct timeval tv;
    tv.tv_sec = TIMEOUT;
    
    int clientSock;
    
    struct pkt packet;
    
    slen=sizeof(struct sockaddr_in);
    
    sock = makeSocket();
    
    FD_ZERO(&fullFdSet);
    
    /* Select process-mode */
    system("clear");
    printf("Select Mode: 1 for server, 2 for client: ");
    
    FD_SET(STDIN_FILENO,&fullFdSet); // Add stdin to active set
    
    while((mode != SERVER)&&(mode != CLIENT)){
        
        printf("");
        select(FD_SETSIZE, &fullFdSet, NULL, NULL, NULL);
        fgets(buffer, sizeof(buffer), stdin);
        size_t ln = strlen(buffer)-1;
        if (buffer[ln] == '\n') buffer[ln] = '\0';
        mode = atoi(buffer);
        if(mode != SERVER && mode != CLIENT)printf("> invalid mode\n");
        
    }
    
    /* Mode specific startup */
    
    if(mode == SERVER){
        
        printf("Mode [%d]: SEVER\n\n",mode);
        bindSocket(sock);
        stateDatabase[sock] = WAITING + SYN;
        FD_CLR(STDIN_FILENO,&fullFdSet); // Remove stdin from active set
        
    }
    
    else if(mode == CLIENT){
        
        printf("Mode [%d]: CLIENT\n\n",mode);
        connectTo(SRV_IP);
        printf("[%d] Sending SYN!\n",sock);
        sendSignal(sock,SYN);
        stateDatabase[sock] = WAITING + SYN + ACK;
        
    }
    
    while(1){ /* Start: Main process-loop */
        
        FD_ZERO(&readFdSet);
        
        readFdSet = fullFdSet;
        
        select(FD_SETSIZE, &readFdSet, NULL, NULL, &tv);
        
        if(FD_ISSET(STDIN_FILENO,&readFdSet)){
            
            FD_CLR(STDIN_FILENO,&readFdSet);
            fgets(buffer, sizeof(buffer), stdin);
            size_t ln = strlen(buffer)-1;
            if (buffer[ln] == '\n') buffer[ln] = '\0';
            if(ln>0)queueSerie(createSerie(buffer), &head);

        };

        
        // Loop ALL sockets
        for(i = sock;i < FD_SETSIZE;i++){ /* Start: ALL sockets if-statement */
            
            if(FD_ISSET(i,&fullFdSet)){ /* Start: ACTIVE sockets if-statement */
                

                
                switch (stateDatabase[i]) { /* Start: State-machine switch */
                        
                    case WAITING + SYN: // Main server state
                        
                        signal = readPacket(i,&packet);
                        
                        if (signal == SYN){
                            
                            clientSock = newClient();
                            
                            printf("[%d] Sending SYNACK!\n",clientSock);
                            sendSignal(clientSock,SYN + ACK);

                            waitTimes[clientSock] = 0;
                            stateDatabase[clientSock] = WAITING + ACK;
    
                        }
                        
                        else {
                            
                            //printf("[%d] waiting\n",i);

                            
                        }
                        
                        break;
                        
                    case WAITING + SYN + ACK:
                        
                        signal = readPacket(i,&packet);
                        
                        if (signal == SYN + ACK){
                            printf("[%d] Sending ACK!\n",i);
                            sendSignal(i,ACK);
                            stateDatabase[i] = ESTABLISHED;
                            printf("[%d] ESTABLISHED\n",i);
                            sleep(1);
                            FD_SET(STDIN_FILENO,&fullFdSet);
                            
                        }
                        
                        else{
                            printf("connection timedout...\n");
                            printf("[%d]Sending SYN!\n",i);
                            sendSignal(i,SYN);
                            
                        }
                        
                        break;
                        
                    case WAITING + ACK:
                        
                        waitTimes[i]++;
                        
                        signal = readPacket(i,&packet);
                        
                        if(waitTimes[i] >= NWAITS){
                            
                            printf("[%d] connection from client lost\n",i);
                            close(i);
                            FD_CLR(i,&fullFdSet);
                            
                        }
                        else if (signal == ACK){
                            
                            printf("[%d] ESTABLISHED\n",i);
                            stateDatabase[i] = ESTABLISHED;
                            
                        }
                        else{
                            
                            printf("[%d] Sending new SYNACK!\n",i);
                            sendSignal(i, SYN + ACK);

                        }
                        
                        break;
                        
                    case ESTABLISHED:
                        
                        signal = readPacket(i,&packet);

                        switch (signal) { /* Start: established state-machine */
                                
                            case SYN + ACK: // If the client gets an extra SYNACK after established
                                sendSignal(i,ACK);
                                break;
                            
                            case NOSIG:
                                
                                break;
                                
                            case DATA + SYN:
                                
                                printf("[%d] %c",i,packet.data);
                                
                                break;
                                
                            case DATA:
                                
                                printf("%c",packet.data);
                                
                                break;
                                
                            case DATA + FIN:
                                
                                printf("%c\n",packet.data);
                                
                                break;
                                
                            case DATA + ACK:
                                
                                break;
                                
                            default:
                                
                                break;
                                
                        } /* End: established state-machine */
                        
                        break;
                        
                    default:
                        
                        printf("[%d] NO STATE: %d\n",i,stateDatabase[i]);
                        
                        break;
                        
                    
                } /* End: state-machine switch */
                
            } /* End: ACTIVE sockets if-statement */
            
        }/* End: ALL sockets if-statement */
        
    } /* End: Main process-loop */
    
    return 0;
    
} /* End: Main Function */

double timestamp(void){
    
    struct timespec tp;
    
    clock_gettime(CLOCK_MONOTONIC, &tp);
    
    double timestamp = tp.tv_sec + (tp.tv_nsec*0.000000001);
    
    return timestamp;

}

struct serie *createSerie(char* input){
    
    struct serie *newSerie = (struct serie*)malloc(sizeof(struct serie));
    
    // Create serie-node
    newSerie->serie = timestamp();
    strcpy(newSerie->data, input);
    newSerie->next = NULL;
    newSerie->index = -1;
    
    return newSerie;
}

void queueSerie(struct serie *newSerie,struct serie **serieHead){
    
    if(*serieHead == NULL)*serieHead = newSerie;
    else queueSerie(newSerie,&((*serieHead)->next));
    

}

int makeSocket(void){
    
    int newSocket;
    struct timeval tv;
    
    // Create the UDP socket
    if ((newSocket=socket(AF_INET, SOCK_DGRAM, 0)) < 0)exit(EXIT_FAILURE);
    
    tv.tv_sec = 1;
    
    setsockopt(newSocket, SOL_SOCKET, SO_RCVTIMEO,(struct timeval *)&tv,sizeof(struct timeval));
    
    return newSocket;
}

int newClient(){
    
    int clientSock;
    
    clientSock = makeSocket();
    
    remotehost[clientSock].sin_family = remotehost[sock].sin_family;
    
    remotehost[clientSock].sin_port = remotehost[sock].sin_port;
    
    remotehost[clientSock].sin_addr = remotehost[sock].sin_addr;
    
    printf("[%d] new client on [%d]\n",sock,clientSock);
    
    FD_SET(clientSock,&fullFdSet);
    
    return clientSock;
    
}

int connectTo(char* server){
    
    memset((char *) &remotehost, 0, sizeof(struct sockaddr_in));
    
    remotehost[sock].sin_family = AF_INET;
    
    remotehost[sock].sin_port = htons(PORT);
    
    if (inet_aton(server, &remotehost[sock].sin_addr)==0) {
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

int sendSignal(int client,int signal){
    
    struct pkt packet;
    
    memset((char *) &packet, 0, sizeof(packet));
    
    packet.flg = signal;
    
    sleep(SENDDELAY);
    
    if(mode == SERVER){
        if (sendto(client, (void*)&packet, sizeof(struct pkt), 0, (struct sockaddr*)&remotehost[client], slen)==-1)exit(EXIT_FAILURE);
    }
    else{
        if (sendto(sock, (void*)&packet, sizeof(struct pkt), 0, (struct sockaddr*)&remotehost[sock], slen)==-1){
            printf("Nåt fel!!!!");
            exit(EXIT_FAILURE);
        }
    }
    
    
    return 1;
}

int readPacket(int client,struct pkt *packet){
    
    memset((char *) packet, 0, sizeof(struct pkt));
    
    if(FD_ISSET(client,&readFdSet)){
        
        recvfrom(client, (void*)packet, sizeof(struct pkt), 0, (struct sockaddr*)&remotehost[client],&slen);
        
        return packet->flg;
        
    }
    
    return NOSIG;

    
}
