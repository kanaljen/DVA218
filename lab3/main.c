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
#include <errno.h>
#include <netdb.h>
#include <time.h>
#include <sys/select.h>
#include <sys/fcntl.h>

struct sockaddr_in localhost, remotehost[FD_SETSIZE];
int sock;
socklen_t slen;
fd_set readFdSet, fullFdSet;
int mode = 0;
struct serie *rcvHead[FD_SETSIZE];

int main(int argc, const char * argv[]) {
    
    int signal = NOSIG, i, k = 0;
    int stateDatabase[FD_SETSIZE];
    int waitTimes[FD_SETSIZE];
    int connectionState[FD_SETSIZE];
    struct serie *newSerie;
    struct serie *sendHead = NULL;
    
    char buffer[512];
    
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 100000;
    
    int clientSock;
    
    struct pkt *packet;
    
    slen=sizeof(struct sockaddr_in);
    
    sock = makeSocket();
    
    FD_ZERO(&fullFdSet);
    
    /* Select process-mode */
    system("clear");
    printf("Select Mode: 1 for server, 2 for client: ");
    
    FD_SET(STDIN_FILENO,&fullFdSet); // Add stdin to active set
    
    while((mode != SERVER)&&(mode != CLIENT)){
        //select(FD_SETSIZE, &fullFdSet, NULL, NULL, NULL);
        fgets(buffer, sizeof(buffer), stdin);
        size_t ln = strlen(buffer)-1;
        if (buffer[ln] == '\n') buffer[ln] = '\0';
        mode = atoi(buffer);
        if(mode != SERVER && mode != CLIENT)printf("> invalid mode\n");
        
    }
    
    /* Mode specific startup */
    FD_CLR(STDIN_FILENO,&fullFdSet); // Remove stdin from active set
    
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
        
        packet = calloc(1, sizeof(struct pkt));
        
        readFdSet = fullFdSet;
        
        select(FD_SETSIZE, &readFdSet, NULL, NULL, &tv);
        
        //  if(FD_ISSET(STDIN_FILENO,&readFdSet) && k == 0){
        if(k == 1 && mode == CLIENT){
            
            FD_CLR(STDIN_FILENO,&readFdSet);
            fgets(buffer, sizeof(buffer), stdin);
            if(!strcmp(buffer,"quit")){
                waitTimes[sock] = 0;
                stateDatabase[sock] = WAITING + FIN +ACK;
            }
            int ln = (int)strlen(buffer)-1;
            if (buffer[ln] == '\n')
                buffer[ln] = '\0';
            if(ln>0){
                newSerie = createSerie(buffer, packet);
                queueSerie(newSerie, &sendHead);
                printf("DETTA SKREV JAG IN: %s\n", sendHead->data);
                printf("Längd = %d \n", sendHead->len);
                sendData(sock, sendHead);
                k ++;
                printf("HEAD LPS EFTER FÖRSTA SÄNDNINGEN = %d\n", sendHead->LPS);
            }
            
        }
        
        
        // Loop ALL sockets
        for(i = sock;i < FD_SETSIZE;i++){ /* Start: ALL sockets if-statement */
            
            if(FD_ISSET(i,&fullFdSet)){ /* Start: ACTIVE sockets if-statement */
                
                
                
                switch (stateDatabase[i]) { /* Start: State-machine switch */
                        
                    case WAITING + SYN: // Main server state
                        
                        signal = readPacket(i,packet);
                        
                        if (signal == SYN){
                            
                            clientSock = newClient();
                            
                            waitTimes[clientSock] = 0;
                            connectionState[clientSock] = 0;
                            
                            stateDatabase[clientSock] = WAITING + ACK;
                            
                        }
                        
                        break;
                        
                    case WAITING + SYN + ACK:
                        
                        signal = readPacket(i,packet);
                        
                        if (signal == SYN + ACK){
                            printf("[%d] Sending ACK!\n",i);
                            sendSignal(i,ACK);
                            stateDatabase[i] = ESTABLISHED;
                            printf("[%d] ESTABLISHED\n",i);
                            sleep(1);
                            //FD_SET(STDIN_FILENO,&fullFdSet);
                            
                        }
                        
                        else{
                            printf("[%d] connection timedout...\n",i);
                            printf("[%d] Sending SYN!\n",i);
                            sendSignal(i,SYN);
                            
                        }
                        
                        break;
                        
                    case WAITING + ACK:
                        
                        signal = readPacket(i,packet);
                        
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
                            sleep(1);
                            
                        }
                        
                        break;
                        
                    case ESTABLISHED:
                        k++;
                        
                        
                        signal = readPacket(i,packet);
                        
                        switch (signal) { /* Start: established state-machine */
                                
                            case SYN + ACK: // If the client gets an extra SYNACK after established
                                sendSignal(i,ACK);
                                k++;
                                break;
                                
                            case NOSIG:
                                
                                if(sendHead != NULL){
                                    if(sendHead->hAI == sendHead->len){
                                        newHead(&sendHead);
                                        printf("Newhead i NOSIG\n");
                                    }
                                    //if(sendHead != NULL)sendData(i, sendHead);
                                }
                                
                                break;
                                
                            case FIN:
                                
                                printf("[%d] Sending FINACK!\n",i);
                                sendSignal(i, FIN + ACK);
                                stateDatabase[i] = WAITING + ACK;
                                waitTimes[i] = 1;
                                break;
                                
                            case DATA:
                                
                                if(rcvHead[i] == NULL)  // First packet
                                    rcvHead[i] = createSerie(NULL, packet);
                                readData(i, *packet, rcvHead[i]);
                                
                                printf("Sending ack with seq: %d hAI: %d\n", packet->seq, rcvHead[i]->hAI);
                                
                                printf("Mottaget: %c\n", packet->data);
                                
                                if(packet->seq == rcvHead[i]->len){ // Serie Complete
                                    
                                    printf("[%d] NEWHEAD %s\n",i,rcvHead[i]->data);
                                    newHead(&(rcvHead[i]));
                                }
                                printf("Mottaget hittills: %s\n", rcvHead[i]->data);
                                
                                
                                
                                
                                
                                
                                break;
                                
                            case DATA + ACK:
                                printf("har fått in en ack, packet hai = %d\n", packet->hAI);
                                if(sendHead == NULL){
                                    printf("SLUT\n");
                                    break;
                                }
                                
                                if(packet->serie == sendHead->serie){
                                    
                                    sendHead->window[(packet->seq) % FULLWINDOW] = ACKED;
                                    if(packet->seq == sendHead->len){
                                        if(packet->hAI == (packet->seq % FULLWINDOW)){
                                            printf("NEWHEAD\n");
                                            newHead(&sendHead);
                                            break;
                                        }
                                        else{
                                            printf("Sending data because all data aint Acked... seq = %d hAI = %d\n", packet->seq, packet->hAI);
                                            sendData(sock, sendHead);
                                        }
                                    }
                                    if(packet->seq == sendHead->LPS - WNDSIZE + 1){
                                        moveWindow(sendHead);
                                        printf("ska skicka next index\n");
                                        sendNextIndex(sock, &sendHead);
                                    }
                                    
                                    else{
                                        printf("Fel paket ackat\n");
                                    }
                                    
                                }
                                else
                                    printf("Wrong serie-number\n");
                                
                                
                                break;
                                
                                
                            default:
                                
                                break;
                                
                                break;
                                
                            case WAITING + FIN + ACK:
                                
                                signal = readPacket(i,packet);
                                
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
                        } /* End: established state-machine */
                        
                        break;
                        
                    default:
                        
                        printf("[%d] NO STATE: %d\n",i,stateDatabase[i]);
                        sleep(2);
                        
                        break;
                        
                        
                } /* End: state-machine switch */
                
            } /* End: ACTIVE sockets if-statement */
            
        }/* End: ALL sockets if-statement */
        
        free(packet);
        
    } /* End: Main process-loop */
    
    return 0;
    
} /* End: Main Function */

int checksum(struct pkt packet){
    
    int checksum;
    
    checksum = (int)packet.data;
    
    return checksum;
    
}

long timestamp(void){
    
    struct timespec tp;
    
    clock_gettime(CLOCK_MONOTONIC, &tp);
    
    double timestamp = tp.tv_sec + (tp.tv_nsec*0.000000001)*1000000;
    
    long intstamp = (long)timestamp;
    
    return intstamp;
    
}

int withinWindow(int seq, struct serie *head){
    
    if(((seq % FULLWINDOW) > head->hAI) && ((seq % FULLWINDOW) <= head->hAI + WNDSIZE)){
        printf("Within window first criteria\n");
        return 1;
    }
    else if(head->hAI > (head->hAI + WNDSIZE) % FULLWINDOW){
        if(((seq % FULLWINDOW) <= head->hAI) && ((seq % FULLWINDOW) > (head->hAI + WNDSIZE) % FULLWINDOW)){
            printf("Not within window\n");
            return 0;
        }
        else{
            printf("Within window second criteria\n");
            return 1;
        }
    }
    printf("Not within window\n");
    return 0;
}

void sendNextIndex(int client, struct serie **head){
    
    struct pkt packet;
    printf("LPS innan = %d\n", (*head)->LPS);
    packet.serie = (*head)->serie;
    packet.flg = DATA;
    packet.len = (*head)->len;
    packet.seq = (*head)->LPS + 1;
    packet.data = (*head)->data[packet.seq];
    packet.chksum = checksum(packet);
    
    (*head)->LPS = packet.seq;
    
    printf("sending in sendNextIndex: %d data = %c LPS = %d\n",packet.seq, packet.data, (*head)->LPS);
    sendto(client, (void*)&packet, sizeof(struct pkt), 0, (struct sockaddr*)&remotehost[sock], slen);
    printf("skickad\n");
    sleep(SENDDELAY);
    if(withinWindow((*head)->LPS + 1, *head))
        sendNextIndex(client, head);
}

void sendData(int client,struct serie *head){
    
    int w;
    
    struct pkt packet;
    
    packet.serie = head->serie;
    packet.flg = DATA;
    packet.len = head->len;
    
    for(w = 1;w <= WNDSIZE;w++){
        
        if(head->window[(head->hAI + w) % FULLWINDOW] == NONACKED){
            packet.seq = head->LPS - WNDSIZE + w;
            packet.data = head->data[packet.seq];
            packet.chksum = checksum(packet);
            printf("sending in senddata: %d\n",packet.seq);
            sendto(client, (void*)&packet, sizeof(struct pkt), 0, (struct sockaddr*)&remotehost[sock], slen);
            sleep(SENDDELAY);
            if(head->LPS < packet.seq)
                head->LPS = packet.seq;
        }
    }
}

void ackData(int client, struct pkt packet, struct serie *head){
    
    packet.flg = DATA + ACK;
    packet.hAI = head->hAI;
    sendto(client, (void*)&packet, sizeof(struct pkt), 0, (struct sockaddr*)&remotehost[client], slen);
}

void readData(int client,struct pkt packet,struct serie *head){
    
    if(!((packet.chksum == checksum(packet)) && (packet.serie == head->serie))){
        printf("Wrong checksum or serie...\n");
        return;
    }
    
    if(withinWindow(packet.seq, head)){
        head->data[packet.seq] = packet.data;
        head->window[packet.seq % FULLWINDOW] = ACKED;
        moveWindow(head);
        ackData(client, packet, head);
    }
    else
        printf("Outside window\n");
    
}

void moveWindow(struct serie *head){
    
    if(head->window[(head->hAI + 1) % FULLWINDOW] == NONACKED)
        return;
    
    head->hAI = (head->hAI + 1) % FULLWINDOW;
    
    head->window[head->hAI] = NONACKED;
    
    printf("HeadHai = %d\n", head->hAI);
    
    moveWindow(head);
    
}

/*void moveWindowSender(struct serie *head){
 
 if(sendHead->window[sendHead->hAI] == ACKED)
 
 
 }*/

struct serie *createSerie(char* input, struct pkt *packet){
    
    struct serie *newSerie = (struct serie*)malloc(sizeof(struct serie));
    // Create serie-node
    if(mode == CLIENT){
        newSerie->serie = timestamp();
        strcpy(newSerie->data, input);
        newSerie->len = (int)strlen(newSerie->data) - 1;
        newSerie->LPS = 0;
    }
    else{
        newSerie->len = packet->len;
        newSerie->serie = packet->serie;
    }
    newSerie->next = NULL;
    newSerie->hAI = -1;
    printf("HEad->hai i createSerie = %d\n", newSerie->hAI);
    //int w;
    
    /*  for(w = 0; w<FULLWINDOW;w++){
     newSerie->window[w] = NONACKED;
     }*/
    printf("HEad->hai i createSerie = %d\n", newSerie->hAI);
    return newSerie;
}

void queueSerie(struct serie *newSerie,struct serie **serieHead){
    
    if(*serieHead == NULL)*serieHead = newSerie;
    else queueSerie(newSerie,&((*serieHead)->next));
    
}

void newHead(struct serie **serieHead){
    
    struct serie *serieToRemove = *serieHead;
    
    if((*serieHead)->next == NULL)
        free(serieToRemove);
    else{
        *serieHead = (*serieHead)->next;
        free(serieToRemove);
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
