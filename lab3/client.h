//
//  client.h
//  client
//
//  Created by Kanaljen on 2017-04-24.
//  Copyright © 2017 Kanaljen. All rights reserved.
//

#ifndef client_h
#define client_h

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

#define hostNameLength 50
#define PORT 5555

int sock = 0;
fd_set monitoredIO,readIO;

bool cmdInterpreter(char* cmd);
void printFile(char* file);

#endif /* client_h */
