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
#define FULLWINDOW  3
#define WNDSIZE     1

#define ACKED       1
#define NONACKED    0

struct pkt{
    int flg;                // Signal
    long serie;             // Unique name for a serie of packets
    int seq;                // Index of char sent
    int len;                // Lenth of data, set by sender
    int hAI;              // Highest index sent, recived and acked. Set by reciver.
    char data;
    int chksum;             // int = (int)char
};

struct serie{
    long serie;             // Unique name for a serie of packets
    char data[BUFFSIZE];
    int len;                // Lenth of data, set by sender
    int window[FULLWINDOW]; // For sender, 1: ack recived, for reciver, 1: ack sent
    int hAI;                // Highest index sent, recived and acked. Set by reciver.
    int LPS;                // Last packet sent from sender.
    struct serie *next;
};

int checksum(struct pkt packet);
long timestamp(void);
int withinWindow(int seq, struct serie *head);
struct serie *createSerie(char* input, struct pkt *packet);
void queueSerie(struct serie *newSerie,struct serie **serieHead);
void newHead(struct serie **serieHead);
void sendNextIndex(int client, struct serie **head);
void sendData(int client,struct serie *head);
void ackData(int client, struct pkt packet, struct serie *head);
void readData(int client,struct pkt packet,struct serie *head);
void moveWindow(struct serie *head);
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
