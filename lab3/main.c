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
int randomMode = 0;

int main(int argc, const char * argv[]) {
    
    int signal = NOSIG, i;
    int stateDatabase[FD_SETSIZE];
    int waitTimes[FD_SETSIZE];
    int connectionState[FD_SETSIZE];
    struct head *sendHead = calloc(1, sizeof(struct head));
    sendHead->first = NULL;
    struct serie *rcvHead[FD_SETSIZE];
    struct serie *newSerie;
    //srand(time(NULL));
    char buffer[512];
    long lastSerie = 0;
    
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
        
        fgets(buffer, sizeof(buffer), stdin);
        size_t ln = strlen(buffer)-1;
        if (buffer[ln] == '\n') buffer[ln] = '\0';
        mode = atoi(buffer);
        if(mode != SERVER && mode != CLIENT)printf("> invalid mode\n");
        
    }
    
    /* Mode specific startup */
    FD_CLR(STDIN_FILENO,&fullFdSet); // Remove stdin from active set
    
    if(mode == SERVER){
        
        printf("Mode [%d]: SERVER\n",mode);
        bindSocket(sock);
        stateDatabase[sock] = WAITING + SYN;
        FD_CLR(STDIN_FILENO,&fullFdSet); // Remove stdin from active set
        printf("waiting for connections...\n\n");
        
    }
    
    else if(mode == CLIENT){
        
        printf("Mode [%d]: CLIENT\n\n",mode);
        printf("Do you want errors? 1=y\n");
        fgets(buffer, sizeof(buffer), stdin);
        randomMode = atoi(buffer);
        connectTo(SRV_IP);
        sendSignal(sock,SYN);
        //if(mode == CLIENT)
        stateDatabase[sock] = WAITING + SYN + ACK;
        //else
        //    stateDatabase[sock] = 100 + rand() % 15; // error example
        printf("\nType something and press [RETURN] to send it to the server.\n");
        printf("Type 'quit' to close client.\n\n");
        printf("[%d] Sending SYN!\n",sock);
        
    }
    
    while(1){ /* Start: Main process-loop */
        
        memset(buffer,0,sizeof(buffer));
        
        srand(time(NULL));
        
        FD_ZERO(&readFdSet);
        
        packet = calloc(1, sizeof(struct pkt));
        
        readFdSet = fullFdSet;
        
        select(FD_SETSIZE, &readFdSet, NULL, NULL, &tv);
        
        if(FD_ISSET(STDIN_FILENO,&readFdSet)){
            
            if(mode == CLIENT){
                
                FD_CLR(STDIN_FILENO,&readFdSet);
                fgets(buffer, sizeof(buffer), stdin); // takes input from stdin
                int ln = (int)strlen(buffer)-1;
                if (buffer[ln] == '\n') // sets Return to '\0'
                    buffer[ln] = '\0';
                if(!strcmp(buffer,"quit")){ //type "quit" to disconnect
                    waitTimes[sock] = 0; // resend n times
                    stateDatabase[sock] = WAITING + FIN +ACK; //sets state to "waiting for fin-ack"
                }
                else{
                    if(ln>0){
                        newSerie = createSerie(buffer, packet);
                        printf("Serie Created\n");
                        sendHead->first = queueSerie(newSerie, sendHead->first); //queues the new serie in the list
                        printf("Queue made\n");
                    }
                }
                
            }
        }
        
        
        // Loop ALL sockets
        for(i = sock;i < FD_SETSIZE;i++){ /* Start: ALL sockets if-statement */
            
            if(FD_ISSET(i,&fullFdSet)){ /* Start: ACTIVE sockets if-statement */
                
                
                
                switch (stateDatabase[i]) { /* Start: State-machine switch */
                        
                    case WAITING + SYN: // Main server state
                        
                        signal = readPacket(i,packet); //recieves a signal from client
                        
                        if (signal == SYN){
                            
                            clientSock = newClient();
                            
                            waitTimes[clientSock] = 0;
                            connectionState[clientSock] = DISCONNECTED; //still no connection
                            
                            stateDatabase[clientSock] = WAITING + ACK; //server waiting for acknowledge
                            
                        }
                        
                        break;
                        
                    case WAITING + SYN + ACK: // client waiting for syn-ack
                        
                        signal = readPacket(i,packet);
                        
                        if (signal == SYN + ACK){ // if syn-ack recieved, send ack and we are connected
                            printf("[%d] Sending ACK!\n",i);
                            sendSignal(i,ACK);
                            stateDatabase[i] = ESTABLISHED;
                            printf("[%d] ESTABLISHED\n",i);
                            sleep(1);
                            FD_SET(STDIN_FILENO,&fullFdSet);
                            
                        }
                        
                        else{ // re-send a syn
                            printf("[%d] connection timedout...\n",i);
                            printf("[%d] Sending SYN!\n",i);
                            sendSignal(i,SYN);
                            
                        }
                        
                        break;
                        
                    case WAITING + ACK:
                        
                        signal = readPacket(i,packet);
                        
                        if(waitTimes[i] >= NWAITS){ // if n times re-send, go back to previous state (server)
                            
                            if(connectionState[i] != CONNECTED){ // client is not connected
                                
                                printf("[%d] connection lost\n",i);
                                close(i);
                                FD_CLR(i,&fullFdSet);
                                
                            }
                            else{ // client is not disconnected
                                
                                stateDatabase[i] = ESTABLISHED;
                                
                            }
                            
                            
                        }
                        else if (signal == ACK && connectionState[i] == DISCONNECTED){ // Connection ACK
                            
                            printf("[%d] ESTABLISHED\n",i);
                            stateDatabase[i] = ESTABLISHED;
                            connectionState[i] = CONNECTED;
                            
                        }
                        
                        else if (signal == ACK && connectionState[i] == CONNECTED){ // Disconnect ACK
                            
                            printf("[%d] Client disconnected\n",i);
                            stateDatabase[i] = 0;
                            close(i);
                            FD_CLR(i,&fullFdSet);
                            
                        }
                        
                        else if (connectionState[i] == CONNECTED){ // re-send a fin-ack
                            
                            printf("[%d] Sending FINACK!\n",i);
                            sendSignal(i,FIN + ACK);
                            waitTimes[i]++;
                            sleep(2);
                            
                        }
                        
                        else{ // re-send a syn-ack
                            
                            printf("[%d] Sending SYNACK!\n",i);
                            sendSignal(i, SYN + ACK);
                            waitTimes[i]++;
                            sleep(1);
                            
                        }
                        
                        break;
                        
                    case ESTABLISHED:
                        
                        
                        signal = readPacket(i,packet); // recieves the signal for a packet
                        
                        switch (signal) { /* Start: established state-machine */
                                
                            case SYN + ACK: // If the client gets an extra SYNACK after established
                                sendSignal(i,ACK);
                                break;
                                
                            case NOSIG:
                                
                                if(mode == CLIENT){
                                    if(sendHead->first != NULL){
                                        if(sendHead->first->LPS == sendHead->first->len){ // if all data is sent, requeue the list with newHead()
                                            if(sendHead->first->hAI == (sendHead->first->len % FULLWINDOW))
                                                sendHead->first = newHead(sendHead->first);
                                            else
                                                sendData(sock, sendHead->first); // if timeout, send all data in the window, which is not acknowledged
                                        }
                                        else
                                            sendData(sock, sendHead->first); // if timeout, send all data in the window, which is not acknowledged
                                    }
                                }
                                
                                break;
                                
                            case FIN: // fin recieved. server sends a fin-ack and puts state to waiting for ack
                                
                                printf("[%d] Sending FINACK!\n",i);
                                sendSignal(i, FIN + ACK);
                                stateDatabase[i] = WAITING + ACK;
                                waitTimes[i] = 1;
                                break;
                                
                            case DATA: // data recieved
                                if(packet->serie == lastSerie){
                                    packet->flg = DATA + ACK;
                                    packet->serie = lastSerie;
                                    sendto(i, (void*)packet, sizeof(struct pkt), 0, (struct sockaddr*)&remotehost[i], slen);
                                    break;
                                }
                                
                                if(rcvHead[i] == NULL){  // First packet
                                    newSerie = createSerie(NULL, packet);
                                    rcvHead[i] = newSerie;
                                }
                                readData(i, *packet, rcvHead[i]); // stores data and sends an acknowledge
                                
                                printf("[%d] Recieved: #%d (%c)\n", i, packet->seq,packet->data);
                                if(packet->seq >= rcvHead[i]->len){
                                    if(rcvHead[i]->hAI == rcvHead[i]->len % FULLWINDOW){ // Serie Complete
                                        printf("[%d] Recieved message: %s\n",i,rcvHead[i]->data);
                                        lastSerie = rcvHead[i]->serie;
                                        rcvHead[i] = newHead(rcvHead[i]); // clear the serie
                                    }
                                }
                                
                                break;
                                
                            case DATA + ACK: // client recieves an acknowledge for some data
                                if(sendHead->first == NULL){ // already sent all data and got them acknowledged
                                    printf("Nothing to recieve\n");
                                    break;
                                }
                                printf("#%d acknowledged (%c)\n", packet->seq, packet->data);
                                if(packet->serie == sendHead->first->serie){ // checks if it is the right serie
                                    
                                    sendHead->first->window[(packet->seq) % FULLWINDOW] = ACKED; // acknowledges the data
                                    
                                    if(sendHead->first->LPS == sendHead->first->len){ // if the last data are acknowledged
                                        if(packet->hAI == (sendHead->first->len % FULLWINDOW)){ // if all data are acknowledged
                                            moveWindow(sendHead->first); // reset window
                                            sendHead->first = newHead(sendHead->first); // requeues the series
                                            break;
                                        }
                                    }
                                    if(packet->seq == sendHead->first->LPS - WNDSIZE + 1){ // if the next expected acknowledge is recieved, send the next sequence in line
                                        moveWindow(sendHead->first);
                                        sendNextIndex(sock, &(sendHead->first));
                                    }
                                    else if(sendHead->first->LPS != sendHead->first->len)
                                        sendData(sock, sendHead->first); // re-sends the data which are not acknowledged, because they are probably lost
                                    
                                }
                                else
                                    printf("Wrong serie-number\n");
                                
                                break;
                                
                                
                            default:
                                
                                
                                break;
                                
                        } /* End: established state-machine */
                        
                        break;
                        
                    case WAITING + FIN + ACK: // client wating for fin-ack to close down
                        
                        signal = readPacket(i,packet);
                        
                        if(signal == FIN + ACK || waitTimes[i] >= NWAITS){ // if waited n times, send ack and close down
                            
                            printf("[%d] Sending ACK and closing!\n",i);
                            sendSignal(i, ACK);
                            sleep(SENDDELAY);
                            sendSignal(i, ACK); // sends a second ack, just to make sure that the server got it
                            free(sendHead);
                            free(packet);
                            close(sock);
                            exit(EXIT_SUCCESS);
                            
                        }
                        
                        else{ // resend a fin-ack
                            printf("[%d] Sending FIN!\n",i);
                            sendSignal(i, FIN);
                            waitTimes[i]++;
                            sleep(SENDDELAY);
                        }
                        
                        break;
                        
                    default:
                        
                        printf("[%d] NO STATE: %d\n",i,stateDatabase[i]); // no state
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
    /*
    if(randomMode == 1)
        checksum = ((int)packet.data * packet.seq) + packet.len + (rand() % 2);
    else
     */
    
    checksum = ((int)packet.data * packet.seq) + packet.len;
    
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
    
    if((seq % FULLWINDOW) > head->hAI && (seq % FULLWINDOW) <= head->hAI + WNDSIZE){ // if the sliding window is "closed" (first place has a lower index than the last place)
        return 1;
    }
    else if(head->hAI > (head->hAI + WNDSIZE) % FULLWINDOW){ // if the sliding window is located around the 0 index (first place has a higher index than the last place)
        if(((seq % FULLWINDOW) <= head->hAI) && ((seq % FULLWINDOW) > (head->hAI + WNDSIZE) % FULLWINDOW)){ // if it is outside of the sliding window
            return 0;
        }
        else{
            return 1;
        }
    }
    return 0;
}

void sendNextIndex(int client, struct serie **head){
    
    if((*head)->LPS == (*head)->len) // if the last data in line is sent
        return;
    struct pkt packet;
    packet.serie = (*head)->serie;
    packet.flg = DATA;
    packet.len = (*head)->len;
    if(randomMode == 1)
        packet.seq = (*head)->LPS + (rand() % 2);
    else
        packet.seq = (*head)->LPS + 1; // send the next data in line
    packet.data = (*head)->data[packet.seq]; // send the next data in line
    packet.chksum = checksum(packet);
    
    printf("sending #%d (%c)\n",packet.seq, packet.data);
    sendto(client, (void*)&packet, sizeof(struct pkt), 0, (struct sockaddr*)&remotehost[sock], slen);
    
    (*head)->LPS = packet.seq; // sets the highest sequence sent
    
    sleep(SENDDELAY);
    if(withinWindow((*head)->LPS + 1, *head)) // if there are room in the sliding window to send more data
        sendNextIndex(client, head);
}

void sendData(int client,struct serie *head){
    
    int w, firstTime;
    
    struct pkt packet;
    
    packet.serie = head->serie;
    packet.flg = DATA;
    packet.len = head->len;
    
    if(head->LPS == -1) // just for the first time the client sends, to fill up the sliding window
        firstTime = WNDSIZE;
    else
        firstTime = 0;
    
    for(w = 1;w <= WNDSIZE;w++){
        if(head->window[(head->hAI + w) % FULLWINDOW] == NONACKED){ // if data is not acknowledged, send it
            packet.seq = head->LPS - WNDSIZE + w + firstTime; // firstTime is here to fill up the sliding window the first time the client sends
            if(packet.seq > head->len) // if the sequence is outside our data
                break;
            packet.data = head->data[packet.seq];
            packet.chksum = checksum(packet);
            printf("sending #%d (%c)\n",packet.seq, packet.data);
            
            if(randomMode == 1){
                if ((rand() % 2) == 0)printf("lost #%d (%c)\n",packet.seq, packet.data);
                else sendto(client, (void*)&packet, sizeof(struct pkt), 0, (struct sockaddr*)&remotehost[sock], slen);
                
            }
            else sendto(client, (void*)&packet, sizeof(struct pkt), 0, (struct sockaddr*)&remotehost[sock], slen);
            
            sleep(SENDDELAY);
        }
    }
    if(head->LPS < packet.seq){
        if(packet.seq < head->len)
            head->LPS = packet.seq; // sets the highest sequence sent
        else
            head->LPS = head->len;
    }
}

void ackData(int client, struct pkt packet, struct serie *head){
    
    packet.flg = DATA + ACK;
    packet.hAI = head->hAI;
    sendto(client, (void*)&packet, sizeof(struct pkt), 0, (struct sockaddr*)&remotehost[client], slen);
}

void readData(int client,struct pkt packet,struct serie *head){
    
    if(packet.chksum != checksum(packet) || packet.serie != head->serie){
        printf("Wrong checksum or serie...\n");
        return;
    }
    
    if(packet.seq > head->len)
        return;
    if(head->window[packet.seq % FULLWINDOW] != ACKED){
        if(withinWindow(packet.seq, head)){ // if data is within window, acknowledge and store it
            head->data[packet.seq] = packet.data;
            head->window[packet.seq % FULLWINDOW] = ACKED;
            moveWindow(head);
            ackData(client, packet, head);
        }
    }
    else{ // else just send an acknowledge. we have probably already got it
        printf("Outside window\n");
        ackData(client, packet, head);
    }
    
}

void moveWindow(struct serie *head){
    
    if(head->window[(head->hAI + 1) % FULLWINDOW] == NONACKED)
        return;
    
    head->hAI = (head->hAI + 1) % FULLWINDOW; // moves the sliding window
    
    head->window[head->hAI] = NONACKED; // puts the places outside the window to not acknowledged
    
    moveWindow(head);
    
}

struct serie *createSerie(char* input, struct pkt *packet){
    
    struct serie *newSerie = (struct serie*)malloc(sizeof(struct serie));
    
    if(mode == CLIENT){
        newSerie->serie = timestamp();
        strcpy(newSerie->data, input); // stores the input until everything is sent
        newSerie->len = (int)strlen(newSerie->data) - 1;
        newSerie->LPS = -1;
    }
    else{
        memset(newSerie->data,0,sizeof(newSerie->data));
        newSerie->len = packet->len; // sets the length it will eventually recieve
        newSerie->serie = packet->serie; // sets the name of the serie
    }
    newSerie->next = NULL;
    newSerie->hAI = -1;
    
    return newSerie;
}

struct serie *queueSerie(struct serie *newSerie,struct serie *serieHead){
    
    if(serieHead == NULL){ // if last in line
        return newSerie;
    }
    else
        serieHead->next = queueSerie(newSerie, serieHead->next); // puts the new serie last in line
    return serieHead;
    
}

struct serie *newHead(struct serie *serieHead){
    
    struct serie *serieToRemove = serieHead;
    
    if(serieHead == NULL){ // if the last serie
        return NULL;
    }
    
    else{ // requeue the next serie to first in line
        serieHead = serieHead->next;
        free(serieToRemove);
        if(serieHead == NULL)
            return NULL;
        return serieHead;
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

int newClient(){ // sets the parameters to the connecting client
    
    int clientSock;
    
    clientSock = makeSocket();
    
    remotehost[clientSock].sin_family = remotehost[sock].sin_family;
    
    remotehost[clientSock].sin_port = remotehost[sock].sin_port;
    
    remotehost[clientSock].sin_addr = remotehost[sock].sin_addr;
    
    printf("[%d] New client on [%d]\n",sock,clientSock);
    
    FD_SET(clientSock,&fullFdSet);
    
    return clientSock;
    
}

int connectTo(char* server){ // puts the parameters to connect to server
    
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
