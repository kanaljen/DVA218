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

#define SERVER         1
#define CLIENT         2

/* Define connected status*/

#define DISCONNECTED  0
#define CONNECTED     1

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
#define SENDDELAY   1
#define BUFFSIZE    512
#define FULLWINDOW  7
#define WNDSIZE     3

#define ACKED       1
#define NONACKED    0

struct pkt{
    int flg;                // Signal
    long serie;             // Unique name for a serie of packets
    int seq;                // Index of char sent
    int len;                // Lenth of data, set by sender
    int hAI;                // Highest index sent, recived and acked. Set by reciver.
    char data;
    int chksum;             // int = (int)char
};

struct serie{
    long serie;             // Unique name for a serie of packets
    char data[BUFFSIZE];
    int len;                // Lenth of data, set by sender
    int window[FULLWINDOW]; // For sender, 1: ack recived, for reciver, 1: ack sent
    int hAI;                // Highest index in window sent, recived and acked.
    int LPS;                // Highest sequence number sent from sender.
    struct serie *next;
};

struct head{
    struct serie* first;    // First serie in the list for the client
};

int checksum(struct pkt packet); //calculating checksum
long timestamp(void); //creating a unique name for a serie of packets
int withinWindow(int seq, struct serie *head); //checks if sequence is within window
struct serie *createSerie(char* input, struct pkt *packet); //creates a serie
struct serie *queueSerie(struct serie *newSerie,struct serie *serieHead); //queues series in client
struct serie *newHead(struct serie *serieHead); //requeues the head of sender and reciever
void sendNextIndex(int client, struct serie **head); //Sending next index in line
void sendData(int client,struct serie *head); //Sending full window which is not acked. At start and if timeout
void ackData(int client, struct pkt packet, struct serie *head); //sends an ack to client
void readData(int client,struct pkt packet,struct serie *head); //reads the data recieved
void moveWindow(struct serie *head); //moves the sliding window
int makeSocket(void); //makes socket
int newClient(); //sets a new client connected to the server
int connectTo(char* server); //connects to server
int bindSocket(int sock); //binds socket
int readPacket(int client,struct pkt *packet); //reads a signal
int sendSignal(int socket,int signal); //sends a signal

#endif /* main_h */
