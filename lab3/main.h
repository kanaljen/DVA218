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
#define DAT      1
#define ACK      2
#define SYN      4
#define FIN      8

#define NOSTATE         0
#define WAITING         100
#define ESTABLISHED     200

#define NWAITS      3
#define TIMEOUT     2
#define SENDDELAY   1

struct pkt{
    int flg;
    int seq;
    int wsz;
    char data[512];
    int chksum;
};

int makeSocket(void);
int connectTo(char* server);
int bindSocket(int sock);
int readSignal(int socket);
int sendSignal(int socket,int signal);
int waitFor(int signal);
int signalRecived(int signal);

int readSocket();

#endif /* main_h */
