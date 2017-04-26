//
//  client.c
//  client
//
//  Created by Kanaljen on 2017-04-24.
//  Copyright © 2017 Kanaljen. All rights reserved.
//

#include "client.h"
#include "protocol.h"
#include "sockets.h"

int main(void){
    
    int i;
    char buffer[256];
    
    system("clear");
     
    FD_ZERO(&monitoredIO);
    FD_SET(STDIN_FILENO , &monitoredIO);
    
    printf("\nType a command and press [RETURN]\n");
    printf("Type 'help' to print helpfile, or 'quit' to exit\n\n");

    while(1){
        readIO = monitoredIO;
        if(select(FD_SETSIZE, &readIO, NULL, NULL, NULL) < 0) {
            perror("select failed\n");
            exit(EXIT_FAILURE);
        }
        else{
            for(i = 0; i < FD_SETSIZE; ++i){
                if(FD_ISSET(i,&readIO)){
                    if(i == STDIN_FILENO){ // It is STDIN
                        fgets(buffer, sizeof(buffer), stdin); //Read stdin to buffer
                        size_t ln = strlen(buffer)-1; // Find last index
                        if (buffer[ln] == '\n') // Fix newline
                            buffer[ln] = '\0';
                        if (!cmdInterpreter(buffer)){ // Do menu stuff
                            exit(EXIT_SUCCESS);
                        }
                    }
                    else{ //data on connected socket
                    // do nuffing
                    }
                }
            }

        }
    }
    
    return 0;
}

bool cmdInterpreter(char* cmd){
    
    if(!strcmp(cmd,"quit"))return(false);
    
    else if(!strcmp(cmd,"help")){
        printf("> ");
        printf("printing help\n\n");
        printFile("help"); //print help file
        return(true);
    }
    
    else if(!strcmp(cmd,"close")){
        
        if (sock == 0){
            printf("> ");
            printf("socket already closed\n\n");
        }
        else{
            close(sock); // Close the socket
            printf("> ");
            printf("socket %d closed\n\n",sock);
            FD_CLR(sock , &monitoredIO); // Remove socket from set
            sock = 0;
        }
        
        return(true);
    }
    
    else if(!strcmp(cmd,"open")){
        if (sock != 0){
            printf("> ");
            printf("socket already open\n\n");
        }
        else{
            sock = createUdpSocket(); // Create a new socket
            printf("> ");
            printf("socket %d opened\n\n",sock);
            FD_SET(sock , &monitoredIO); // Add socket to set
            
        }
        
        return(true);
    }
    
    else if(!strcmp(cmd,"clear")){
        system("clear");
        return(true);
    }
    
    else if(!strcmp(cmd,"send")){
        printf("> ");
        printf("send packets\n\n");
        //Send data placeholder
        return(true);
    }
    
    //Unrecognized command
    
    else{
        printf("> ");
        printf("command not found\n\n");
        return(true);
    }
    
};

void printFile(char* file){
    
    char buffer[1024];
    FILE *fp = fopen(file,"r+");
    if(fp == NULL){
        printf("error printing file\n\n");
    }
    else{
        while(fgets(buffer, sizeof(buffer), fp)){
            printf("%s",buffer);
        }
        printf("\n");
    }

    fclose(fp);
    
};


