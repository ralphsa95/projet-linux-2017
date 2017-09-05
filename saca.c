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
#include <pthread.h>
#include "saca.h"


int planesSockets[300];
int controlsSockets[300];
int planeCount = 0;
int contCompteur = 0;
int readFromPLanes;
int ecouterCont;
char **nomAvions = 0;


typedef struct 
{
    int x;
    int y;
    int a;
    int c;
    int v;
    int controlleur;
}AvionSaca;



AvionSaca *avions = 0;

void listenToPLanesFunc(void *param)
{
  int x, y, a, v, c;
  char buffer[1024] = {0};
  while(1)
  {
    for(int i=0; i< planeCount; i++)
    {
      readFromPLanes = read(planesSockets[i],  buffer, 1024);
      if(readFromPLanes > 0)
      {
        split(buffer,5,&x,&y,&a,&v,&c);
        avions[i].x = x;
        avions[i].y = y;
        avions[i].a = a;
        avions[i].v = v;
        avions[i].c = c; 
       
       printf("Avion %s -> localisation : (%d,%d), altitude : %d, vitesse : %d, cap : %d\n",
            nomAvions[i], avions[i].x, avions[i].y, avions[i].a, avions[i].v, avions[i].c);
      }
    }
  } 
}


void connecter_avions(void *param){

    
    int server_fd, new_socket, valread;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};
    pthread_t listenToPLanes;


    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
     

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr("127.0.0.1");
    address.sin_port = htons( PORT );

    if (bind(server_fd, (struct sockaddr *)&address, 
                                 sizeof(address))<0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 300) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    int x,y,a,v,c;
    int comaCounter = 0;
    int counter = 0;
    char *bufferx;
    while(1)
    {
	if ((new_socket = accept(server_fd, (struct sockaddr *)&address,(socklen_t*)&addrlen))<0)
	{
	  perror("accept");
	  exit(EXIT_FAILURE);
	}
	planesSockets[planeCount] = new_socket;
        read(new_socket,  buffer, 1024); 
        nomAvions[planeCount] = (char*)malloc(sizeof(char)*5);
        strncpy( nomAvions[planeCount], (char*)buffer, 5);
       
        split(buffer,5,&x,&y,&a,&v,&c);
        avions[planeCount].x = x;
        avions[planeCount].y = y;
        avions[planeCount].a = a;
        avions[planeCount].v = v;
        avions[planeCount].c = c;
        printf("Avion %s -> localisation : (%d,%d), altitude : %d, vitesse : %d, cap : %d\n",
             nomAvions[planeCount], avions[planeCount].x, avions[planeCount].y,
             avions[planeCount].a, avions[planeCount].v, avions[planeCount].c);
       
        struct timeval tv; tv.tv_sec = 0; tv.tv_usec = 20000;
        setsockopt(new_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv,sizeof(struct timeval));
        
        if (planeCount == 0 )
        {
          pthread_create(&listenToPLanes, NULL, listenToPLanesFunc, NULL); 
        }
        planeCount++;
    }
}

void ecouterControlleur(void *param)
{
  char buffer[1024] = {0};
  char nom [5] = {0};
  int index = -1;
  while(1)
  {
    for(int i=0; i< contCompteur; i++)
    {
      ecouterCont = read(controlsSockets[i],  buffer, 1024);
      if(ecouterCont > 0)
      {
        
        strncpy(nom, (char*)buffer, 5);
        index = -1;
        for(int j =0; j<planeCount; j++)
        {
          if(strcmp(nomAvions[j], nom) == 0)
          {
            index = j;
            break;
          }
        }
        //
        if(index == -1)
        {
             send(controlsSockets[i], "Avion inconnu", 1024, 0);
        }
        else if(buffer[5] == '-' && (buffer[6] == 'v' 
                                  || buffer[6] == 'c' 
                                  || buffer[6] == 'a'))
        {
            memmove(buffer, buffer+6, strlen(buffer));
            send(controlsSockets[i], "Succes", 1024, 0);
            send(planesSockets[index], buffer, 1024, 0);
            
        }
        else
        {
          send(controlsSockets[i], "Commande pas bien ecrite exemple: MEE-a=20", 1024, 0);
        } 
      }

    }
  } 
}

void connecter_controlleurs(void *param){
    
    int server_fd, new_socket, valread;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};
    pthread_t ecouter_controlleurs;


    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
      
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr("127.0.0.1");
    address.sin_port = htons( PORTC );

    if (bind(server_fd, (struct sockaddr *)&address, 
                                 sizeof(address))<0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 300) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }
   
    while(1)
    {
	if ((new_socket = accept(server_fd, (struct sockaddr *)&address,(socklen_t*)&addrlen))<0)
	{
	  perror("accept");
	  exit(EXIT_FAILURE);
	}
	controlsSockets [contCompteur] = new_socket;
	//
        struct timeval tv; tv.tv_sec = 0; tv.tv_usec = 20000; 
        setsockopt(new_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv,sizeof(struct timeval));
        if (contCompteur == 0 )
        {
          pthread_create(&ecouter_controlleurs, NULL, ecouterControlleur, NULL); 
        }
        contCompteur++;
    }
}

void split(char *buffer, int index, int *x, int *y, int *a, int *v,int *c)
{
  int comaCounter =0;
  int counter =0;
  char *xx = (char*)malloc(sizeof(char)*5);
  char *yy = (char*)malloc(sizeof(char)*5);
  char *aa = (char*)malloc(sizeof(char)*5);
  char *cc = (char*)malloc(sizeof(char)*5);
  char *vv = (char*)malloc(sizeof(char)*5);
  for(int i=index; i< strlen(buffer); i++)
  {   
     
    if(buffer[i] == ',')
    {
      comaCounter++;
      counter = 0;
    }
    else 
    {
      if(comaCounter == 1)
      {
	xx[counter] = buffer[i]; 
      }
      else if (comaCounter == 2)
      {
	yy[counter] = buffer[i]; 
      }   
      else if (comaCounter == 3)
      {
	aa[counter] = buffer[i]; 
      }  
      else if (comaCounter == 4)
      {
	vv[counter] = buffer[i]; 
      }   
      else if (comaCounter == 5)
      {
	cc[counter] = buffer[i]; 
      } 
      counter++;   
    }

  }  
  *x=atoi(xx);
  *y=atoi(yy);
  *a=atoi(aa);
  *v=atoi(vv);
  *c=atoi(cc);
}

int main(int argc, char const *argv[])
{   

  pthread_t connectAvions;
  pthread_create(&connectAvions, NULL, connecter_avions, NULL);
  nomAvions = (char**)malloc(sizeof(char*)*300);
  avions = (AvionSaca*)malloc(sizeof(AvionSaca)*300);
  pthread_t connectControlleurs;
  pthread_create(&connectControlleurs, NULL, connecter_controlleurs, NULL);
  
  getchar();
  return 0;


}
