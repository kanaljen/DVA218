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
struct serie *sendHead = NULL,*rcvHead[FD_SETSIZE];

int main(int argc, const char * argv[]) {
    
    int signal = NOSIG, i;
    int stateDatabase[FD_SETSIZE];
    int waitTimes[FD_SETSIZE];
    int connectionState[FD_SETSIZE];
    
    char buffer[512];
    
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 100000;
    
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
        
        printf("Mode [%d]: SEVER\n",mode);
        bindSocket(sock);
        stateDatabase[sock] = WAITING + SYN;
        FD_CLR(STDIN_FILENO,&fullFdSet); // Remove stdin from active set
        printf("waiting for connections...\n\n");
        
    }
    
    else if(mode == CLIENT){
        
        printf("Mode [%d]: CLIENT\n\n",mode);
        connectTo(SRV_IP);
        sendSignal(sock,SYN);
        stateDatabase[sock] = WAITING + SYN + ACK;
        printf("\nType something and press [RETURN] to send it to the server.\n");
        printf("Type 'quit' to close client.\n\n");
        printf("[%d] Sending SYN!\n",sock);
        
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
            if(!strcmp(buffer,"quit")){
                waitTimes[sock] = 0;
                stateDatabase[sock] = WAITING + FIN +ACK;
            }
            else if(ln>0)queueSerie(createSerie(buffer), &sendHead);
            
        };
        
        
        // Loop ALL sockets
        for(i = sock;i < FD_SETSIZE;i++){ /* Start: ALL sockets if-statement */
            
            if(FD_ISSET(i,&fullFdSet)){ /* Start: ACTIVE sockets if-statement */
                
                
                
                switch (stateDatabase[i]) { /* Start: State-machine switch */
                        
                    case WAITING + SYN: // Main server state
                        
                        signal = readPacket(i,&packet);
                        
                        if (signal == SYN){
                            
                            clientSock = newClient();
                            
                            waitTimes[clientSock] = 0;
                            connectionState[clientSock] = 0;
                            
                            stateDatabase[clientSock] = WAITING + ACK;
                            
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
                            printf("[%d] connection timedout...\n",i);
                            printf("[%d] Sending SYN!\n",i);
                            sendSignal(i,SYN);
                            
                        }
                        
                        break;
                        
                    case WAITING + ACK:
                        
                        signal = readPacket(i,&packet);
                        
                        if(waitTimes[i] >= NWAITS){
                            
                            if(connectionState[i] != 1){
                                
                                printf("[%d] connection lost\n",i);
                                close(i);
                                FD_CLR(i,&fullFdSet);
                                
                            }
                            else{
                                
                                stateDatabase[i] = ESTABLISHED;
                                
                            }

                            
                        }
                        else if (signal == ACK && connectionState[i] != 1){ // Connection ACK
                            
                            printf("[%d] ESTABLISHED\n",i);
                            stateDatabase[i] = ESTABLISHED;
                            connectionState[i] = 1;
                            
                        }
                        
                        else if (signal == ACK && connectionState[i] == 1){ // Disconnect ACK
                            
                            printf("[%d] Client disconnected\n",i);
                            stateDatabase[i] = 0;
                            close(i);
                            FD_CLR(i,&fullFdSet);
                            
                        }
                        
                        else if (connectionState[i] == 1){
                            
                            printf("[%d] Sending FINACK!\n",i);
                            sendSignal(i,FIN + ACK);
                            waitTimes[i]++;
                            
                        }
                        
                        else{
                            
                            printf("[%d] Sending SYNACK!\n",i);
                            sendSignal(i, SYN + ACK);
                            waitTimes[i]++;
                            
                        }
                        
                        break;
                        
                    case ESTABLISHED:
                        

                        
                        signal = readPacket(i,&packet);
                        
                        switch (signal) { /* Start: established state-machine */
                                
                            case SYN + ACK: // If the client gets an extra SYNACK after established
                                sendSignal(i,ACK);
                                break;
                                
                            case NOSIG:
                                
                                if(sendHead != NULL){
                                    if(sendHead->index == sendHead->len -1)sendHead = newHead(sendHead);
                                    if(sendHead != NULL)sendData(i, sendHead);
                                }
                                
                                break;
                                
                            case FIN:
                                
                                printf("[%d] Sending FINACK!\n",i);
                                sendSignal(i, FIN + ACK);
                                stateDatabase[i] = WAITING + ACK;
                                waitTimes[i] = 1;
                                break;
                                
                            case DATA:
                                
                                if(rcvHead[i] == NULL){  // First packet
                                    rcvHead[i] = createSerie("");
                                    rcvHead[i]->serie = packet.serie;
                                    readData(i, packet, rcvHead[i]);
                                    
                                }

                                else{  // Read data to packet
                                    readData(i, packet, rcvHead[i]);
                                    
                                    if(rcvHead[i]->index == rcvHead[i]->len -1){ // Serie Complete
                                        
                                        printf("[%d] %s\n",i,rcvHead[i]->data);
                                        rcvHead[i] = newHead(rcvHead[i]);
                                        
                                        
                                    }
                                    

                                    
                                }
                            
                                
                                break;
                                
                            case DATA + ACK:
                                
                                if(sendHead == NULL)break;
                                
                                if(packet.serie == sendHead->serie){
                                    
                                    sendHead->window[(packet.seq - 1) - sendHead->index] = 1;
                                    
                                    moveWindow(packet.index - sendHead->index, sendHead);
                                    
                                    sendHead->index = packet.index;
                                    
                                    if(sendHead->index == sendHead->len -1)sendHead = newHead(sendHead);
                                    
                                }
                                
                                
                                break;
                                
                                
                            default:
                                
                                break;
                                
                        } /* End: established state-machine */
                        
                        break;
                        
                        case WAITING + FIN + ACK:
                        
                        signal = readPacket(i,&packet);
                        
                        if(signal == FIN + ACK || waitTimes[i] >= NWAITS){
                            
                            printf("[%d] Sending ACK and closing!\n",i);
                            sendSignal(i, ACK);
                            close(sock);
                            exit(EXIT_SUCCESS);
                            
                        }
                        
                        else{
                            printf("[%d] Sending FIN!\n",i);
                            sendSignal(i, FIN);
                            waitTimes[i]++;
                        }
                        
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

long timestamp(void){
    
    struct timespec tp;
    
    clock_gettime(CLOCK_MONOTONIC, &tp);
    
    double timestamp = tp.tv_sec + (tp.tv_nsec*0.000000001)*1000000;
    
    long intstamp = (long)timestamp;
    
    return intstamp;
    
}

void sendData(int client,struct serie *serie){
    
    int w;
    
    struct pkt packet;
    
    memset((char *) &packet, 0, sizeof(packet));
    
    packet.serie = serie->serie;
    packet.flg = DATA;
    packet.len = serie->len;
    packet.index = serie->index;
    
    for(w = 0;w<WNDSIZE;w++){
        
        if(serie->window[w] == 0){
            
            if(packet.index+w+1 > serie->len)break;
            
            packet.data = serie->data[packet.index+w+1];
            packet.chksum = (int)packet.data;
            packet.seq = packet.index+w+1;
            
            printf("seding: %d\n",packet.seq);
            sendto(sock, (void*)&packet, sizeof(struct pkt), 0, (struct sockaddr*)&remotehost[sock], slen);
            sleep(SENDDELAY);
            
        }
    }
    
}

void readData(int client,struct pkt packet,struct serie *head){
    
    if(packet.chksum != (int)packet.data);
    //else if (packet.serie != head->serie);
    else if (packet.seq < head->index || packet.seq >= head->index + WNDSIZE);
    else{
        head->data[packet.seq] = packet.data;
        head->len = packet.len;
        head->window[(packet.seq - head->index) - 1] = 1;
        packet.flg = DATA + ACK;
        
        int w,steps = 0;
        
        for(w = 0;w<WNDSIZE;w++){
            
            if(head->window[w] == 0)break;
            else steps++;
            
        }
        
        moveWindow(steps,head);
        
        head->index = head->index + steps;
        packet.index = head->index;
        
        sendto(client, (void*)&packet, sizeof(struct pkt), 0, (struct sockaddr*)&remotehost[client], slen);
        
    }
}

void moveWindow(int steps,struct serie *head){
    
    int w;
    
    for(w = 0;w<WNDSIZE;w++){
        
        if(w < WNDSIZE - steps)head->window[w] = head->window[w+steps];
        else head->window[w] = 0;

    }
    
}

struct serie *createSerie(char* input){
    
    struct serie *newSerie = (struct serie*)malloc(sizeof(struct serie));
    
    // Create serie-node
    newSerie->serie = timestamp();
    strcpy(newSerie->data, input);
    newSerie->len = (int)strlen(newSerie->data);
    newSerie->next = NULL;
    newSerie->index = -1;
    
    int w;
    
    for(w = 0; w<WNDSIZE;w++){
        newSerie->window[w] = 0;
    }
    
    return newSerie;
}

void queueSerie(struct serie *newSerie,struct serie **serieHead){
    
    if(*serieHead == NULL)*serieHead = newSerie;
    else queueSerie(newSerie,&((*serieHead)->next));
    
}

struct serie *newHead(struct serie *serieHead){
    
    if(serieHead == NULL)return NULL;
    
    else{
        struct serie *temp = serieHead->next;
        free(serieHead);
        return temp;
    }
    
}

int makeSocket(void){
    
    int newSocket;
    struct timeval tv;
    
    // Create the UDP socket
    if ((newSocket=socket(AF_INET, SOCK_DGRAM, 0)) < 0)exit(EXIT_FAILURE);
    
    tv.tv_sec = 0;
    tv.tv_usec = 100000;
    
    setsockopt(newSocket, SOL_SOCKET, SO_RCVTIMEO,(struct timeval *)&tv,sizeof(struct timeval));
    
    return newSocket;
}

int newClient(){
    
    int clientSock;
    
    clientSock = makeSocket();
    
    remotehost[clientSock].sin_family = remotehost[sock].sin_family;
    
    remotehost[clientSock].sin_port = remotehost[sock].sin_port;
    
    remotehost[clientSock].sin_addr = remotehost[sock].sin_addr;
    
    printf("[%d] New client on [%d]\n",sock,clientSock);
    
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
