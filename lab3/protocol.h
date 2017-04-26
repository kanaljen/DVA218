//
//  protocol.h
//  client
//
//  Created by Kanaljen on 2017-04-25.
//  Copyright © 2017 Kanaljen. All rights reserved.
//

#ifndef protocol_h
#define protocol_h

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

/* Define protocol flags */

#define DAT 1   // Data
#define ALI 2   // Alive
#define SYN 4   // Synchronize
#define ACK 8   // Acknowledge
#define FIN 16  // Finish
#define REF 32  // Refuse

/* Protocol packet-struct */

struct protocolPacket{
    int flg;    // Flag or flags
    int seq;    // Sequence number
    int csm;    // Checksum
    int wsz;    // Sender window size
    void* dat;  // Data
}packet;

/* Checksum functions */

int calChecksum(); // Function for calculating the checksum, returns checksum
bool cmpChecksum(); // Check checksum of recived packet
bool hostAlive(int sock); // Send alive packet to host

#endif /* protocol_h */
