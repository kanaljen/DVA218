//
//  sockets.c
//  client
//
//  Created by Kanaljen on 2017-04-26.
//  Copyright © 2017 Kanaljen. All rights reserved.
//

#include "sockets.h"



int createUdpSocket(void){
    
    int sock = socket(PF_INET, SOCK_DGRAM, 0);
    
    if(sock < 0) {
        perror("Could not create a socket\n");
        exit(EXIT_FAILURE);
    }

    return sock;
    
}

void initSocket(struct sockaddr_in *name, char *hostName, unsigned short int port) {
    
    //Struct to fill with info about the server
    struct hostent *hostInfo;
    
    /* Socket address format set to AF_INET for Internet use. */
    name->sin_family = AF_INET;
    
    /* Set port number. The function htons converts from host byte order to network byte order.*/
    name->sin_port = htons(port);
    
    /* Get info about host. */
    hostInfo = gethostbyname(hostName);
    if(hostInfo == NULL) {
        fprintf(stderr, "initSocketAddress - Unknown host %s\n",hostName);
        exit(EXIT_FAILURE);
    }
    
    /* Fill in the host name into the sockaddr_in struct. */
    name->sin_addr = *(struct in_addr *)hostInfo->h_addr;
    
}
