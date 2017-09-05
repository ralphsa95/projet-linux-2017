/*
 
 * Ralph SAADE Copyrights
 * CNAM-LIBAN 2017
 
 */

#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <malloc.h>
#include "saca.h"


char x;
int main(int argc, char const *argv[])
{
  struct sockaddr_in address;
  int sock = 0, valread;
  struct sockaddr_in serv_addr;

  
  sock = socket(AF_INET,SOCK_STREAM,0);
  memset(&serv_addr, '0', sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(PORTC);
	
  inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);	
  
  if(connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
  {
    printf("\n Connection Failed");
    return -1;
  }


  while(1)
  {
    char donnee[1024];
    char command[1024];
     
    scanf("%s", command);
    send(sock, command, 1024, 0);
    read(sock, donnee, 1024);
    printf("%s\n", donnee);
  }

}