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


int avionsSockets[300];
int controlsSockets[300];
int avionCompteur = 0;
int contCompteur = 0;
int ecouterAvion;
int ecouterCont;
char **nomAvions = 0;
int limit = 5;
int xdif, ydif, adif;

typedef struct 
{
    int x;
    int y;
    int a;
    int c;
    int v;
    char *control;// = malloc(sizeof(char)*3);
}AvionSaca;



AvionSaca *avions = 0;

void listenToPLanesFunc(void *param)
{
  int x, y, a, v, c;
  char buffer[1024] = {0};
  while(1)
  {
    for(int i=0; i< avionCompteur; i++)
    {
      ecouterAvion = read(avionsSockets[i],  buffer, 1024);
      if(ecouterAvion > 0)
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
    printf("Serveur SACA pres a recevoir les avions \n");
    while(1)
    {
	if ((new_socket = accept(server_fd, (struct sockaddr *)&address,(socklen_t*)&addrlen))<0)
	{
	  perror("accept");
	  exit(EXIT_FAILURE);
	}
	avionsSockets[avionCompteur] = new_socket;
        read(new_socket,  buffer, 1024); 
        nomAvions[avionCompteur] = (char*)malloc(sizeof(char)*5);
        strncpy( nomAvions[avionCompteur], (char*)buffer, 5);
       
        split(buffer,5,&x,&y,&a,&v,&c);
        avions[avionCompteur].x = x;
        avions[avionCompteur].y = y;
        avions[avionCompteur].a = a;
        avions[avionCompteur].v = v;
        avions[avionCompteur].c = c;
        avions[avionCompteur].control = malloc(sizeof(char)*3);
        printf("Avion %s -> localisation : (%d,%d), altitude : %d, vitesse : %d, cap : %d\n",
             nomAvions[avionCompteur], avions[avionCompteur].x, avions[avionCompteur].y,
             avions[avionCompteur].a, avions[avionCompteur].v, avions[avionCompteur].c);
       
        struct timeval tv; tv.tv_sec = 0; tv.tv_usec = 20000;
        setsockopt(new_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv,sizeof(struct timeval));
        
        if (avionCompteur == 0 )
        {
          pthread_create(&listenToPLanes, NULL, listenToPLanesFunc, NULL); 
        }
        avionCompteur++;
    }
}

void ecouterControlleur(void *param)
{
  char buffer[1024] = {0};
  char control[1024] = {0};
  char nom [5] = {0};
  int index = -1;
  while(1)
  {
    for(int i=0; i< contCompteur; i++)
    {
      ecouterCont = read(controlsSockets[i],  buffer, 1024);
      if(ecouterCont > 0)
      {
        strncpy(control, (char*)buffer, 3);
        memmove(buffer, buffer+4, strlen(buffer));
        strncpy(nom, (char*)buffer, 5);
        index = -1;
        for(int j =0; j<avionCompteur; j++)
        {
          if(strcmp(nomAvions[j], nom) == 0)
          {
            index = j;
            if(avions[j].control[0] == '\0') //avion free
            {
                strcpy(avions[j].control, control);
            }
            else if(strcmp(avions[j].control,control) != 0) //avion fonctionne avec autre controlleur
            {
                index = -2; 
            }
            break;
          }
        }
        //
        if(index == -1)
        {
             send(controlsSockets[i], "Avion inconnu", 1024, 0);
        }
        else if(index == -2)
        {
          send(controlsSockets[i], "Cette avion est maitenant bloque avec un autre controlleur", 1024, 0);
        }
        else if(buffer[5] == '-' && (buffer[6] == 'v' 
                                  || buffer[6] == 'c' 
                                  || buffer[6] == 'a'
                                  || buffer[6] == 'l'))
        {
          if(buffer[6] == 'l')
          {
            avions[index].control[0] = '\0';
          }
          else
          {
            memmove(buffer, buffer+6, strlen(buffer));
            send(avionsSockets[index], buffer, 1024, 0);
          }
          send(controlsSockets[i], "Succes", 1024, 0);
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
    
    printf("Serveur SACA pres a recevoir les controlleurs \n");
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
  printf("Serveur SACA demarrer\n");
  pthread_t connectAvions;
  pthread_create(&connectAvions, NULL, connecter_avions, NULL);
  nomAvions = (char**)malloc(sizeof(char*)*300);
  avions = (AvionSaca*)malloc(sizeof(AvionSaca)*300);
  pthread_t connectControlleurs;
  pthread_create(&connectControlleurs, NULL, connecter_controlleurs, NULL);
  
  while(1)
  {
    if(avionCompteur > 1)
    {
      for(int i =0; i< avionCompteur; i++)
      {
        for(int j=1; j< avionCompteur; j++)
        {
          if(i != j)
          {
            xdif = abs(avions[i].x - avions[j].x);
            ydif = abs(avions[i].y - avions[j].y);
            adif = abs(avions[i].a - avions[j].a);
             
            if(xdif <= limit && ydif <= limit && adif <= limit)
            {
              printf("Avion %s et %s vont s'ecarser, veuillez intervenir\n",nomAvions[i],nomAvions[j]);
            }
          }
        }
      }
    }
  }
  return 0;

}
