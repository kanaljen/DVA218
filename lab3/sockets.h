//
//  sockets.h
//  client
//
//  Created by Kanaljen on 2017-04-26.
//  Copyright © 2017 Kanaljen. All rights reserved.
//

#ifndef sockets_h
#define sockets_h

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

int createUdpSocket(void);
void initSocket(struct sockaddr_in *name, char *hostName, unsigned short int port);
//struct protocolPacket readSocket(int sock);
//int writeSocket(struct protocolPacket packet);


#endif /* sockets_h */
