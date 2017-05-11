//
//  main.h
//  DVA218 - lab3
//
//  Created by Kanaljen on 2017-05-01.
//  Copyright © 2017 Kanaljen. All rights reserved.
//

#ifndef main_h
#define main_h

#define PORT 8888
#define SRV_IP "127.0.0.1"

/* Define process mode */
#define SERVER 1
#define CLIENT 2

/* Define machine signals */

#define NOSIG    0
#define DATA     1
#define ACK      2
#define SYN      4
#define FIN      8

/* Define mashine states */

#define NOSTATE         0
#define WAITING         100
#define ESTABLISHED     200

#define NWAITS      3
#define TIMEOUT     1
#define SENDDELAY   0
#define BUFFSIZE    512
#define WNDSIZE     3

struct pkt{
    int flg;
    double serie;
    int seq;
    int len;
    int index;
    char data;
    int chksum;
};

struct serie{
    long serie;
    char data[BUFFSIZE];
    int len;
    int window[WNDSIZE];   // For sender, 1: ack recived, for reciver, 1: ack sent
    int index;              // Last send, recived and acked index. Set by reciver.
    struct serie *next;
};

long timestamp(void);
struct serie *createSerie(char* input);
void queueSerie(struct serie *newSerie,struct serie **serieHead);
struct serie *newHead(struct serie *serieHead);
void sendData(int client,struct serie *serie);
void readData(int client,struct pkt packet,struct serie *head);
void moveWindow(int steps,struct serie *head);
int makeSocket(void);
int newClient();
int connectTo(char* server);
int bindSocket(int sock);
int readPacket(int client,struct pkt *packet);
int sendSignal(int socket,int signal);
int waitFor(int signal);
int signalRecived(int signal);

int readSocket();

#endif /* main_h */
