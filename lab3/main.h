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
#define WINDOW_SIZE 3
#define STREAM_SIZE 3

struct pkt{
    int flg;
    int seq;
    double serie;
    double time;
    int len;
    char data;
    int chksum;
};

double timestamp(void);
int createPacket(char* input);
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
