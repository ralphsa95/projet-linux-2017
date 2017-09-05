/*
 
 * Ralph SAADE Copyrights
 * CNAM-LIBAN 2017
 
 */

#include <math.h>
#include "avion.h"
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "saca.h"



struct deplacement dep;
struct coordonnees coord;
char numero_vol[6];

struct sockaddr_in address;
int sock = 0, valread, lire;
struct sockaddr_in serv_addr;
char donnee[1024];
char command[1024];

int ouvrir_communication() {
	//avions[0] = ;
        sock = socket(AF_INET,SOCK_STREAM,0);
	memset(&serv_addr, '0', sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(PORT);
	
	inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);	

	if(connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		printf("\n Connection Failed");
		return -1;
	}
	else
	{
		//printf("connected");		
		return 1;
	}

 
}

void fermer_communication() {
	close(sock);
}

void envoyer_caracteristiques() {
	
	
sprintf(donnee, "%s,%d,%d,%d,%d,%d",
            numero_vol, coord.x, coord.y, coord.altitude, dep.vitesse, dep.cap);	
send(sock, donnee, 1024, 0);

}

/********************************
 ***  Fonctions gerant le deplacement de l'avion : ne pas modifier
 ********************************/

// initialise aleatoirement les paramétres initiaux de l'avion

void initialiser_avion() {
    int a;

    srand ( time(NULL));
    a = rand(); 
 // intialisation des paramétres de l'avion
    coord.x = 1000 + a % 1000;
    coord.y = 500 + a % 1000;
    coord.altitude = 900 + a % 100;

    dep.cap = a % 360;
    dep.vitesse = 600 + a % 200;
	

    //afficher_donnees();
  
    const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    const char nums[]    = "0123456789";


    for (size_t n = 1; n < 6; n++) 
    {
    	srand ( time(NULL) * n * n);
	a = rand();    
	if(n < 3)
	{       
 		int key = a % (int) (sizeof charset - 1);
            	numero_vol[n-1] = charset[key];
	}
	else
	{
		int key = a % (int) (sizeof nums - 1);
            	numero_vol[n-1] = nums[key];			
	}
    } 
}

// modifie la valeur de l'avion avec la valeur pass�e en param�tre

void changer_vitesse(int vitesse) {
    if (vitesse < 0)
        dep.vitesse = 0;
    else if (vitesse > VITMAX)
        dep.vitesse = VITMAX;
    else dep.vitesse = vitesse;
}

// modifie le cap de l'avion avec la valeur passée en paramètre

void changer_cap(int cap) {
    if ((cap >= 0) && (cap < 360))
        dep.cap = cap;
}

// modifie l'altitude de l'avion avec la valeur passée en paramètre

void changer_altitude(int alt) {
    if (alt < 0)
        coord.altitude = 0;
    else if (alt > ALTMAX)
        coord.altitude = ALTMAX;
    else coord.altitude = alt;
}

// affiche les caractéristiques courantes de l'avion

/*void afficher_donnees() {
    printf("Avion %s -> localisation : (%d,%d), altitude : %d, vitesse : %d, cap : %d\n",
            numero_vol, coord.x, coord.y, coord.altitude, dep.vitesse, dep.cap);
}*/

// recalcule la localisation de l'avion en fonction de sa vitesse et de son cap

void calcul_deplacement() {
    float cosinus, sinus;
    float dep_x, dep_y;
    int nb;

    if (dep.vitesse < VITMIN) {
        printf("Vitesse trop faible : crash de l'avion\n");
        fermer_communication();
        exit(2);
    }
    if (coord.altitude == 0) {
        printf("L'avion s'est ecrase au sol\n");
        fermer_communication();
        exit(3);
    }
    //cos et sin ont un paramétre en radian, dep.cap en degré nos habitudes francophone
    /* Angle en radian = pi * (angle en degré) / 180 
       Angle en radian = pi * (angle en grade) / 200 
       Angle en grade = 200 * (angle en degré) / 180 
       Angle en grade = 200 * (angle en radian) / pi 
       Angle en degré = 180 * (angle en radian) / pi 
       Angle en degré = 180 * (angle en grade) / 200 
     */

    cosinus = cos(dep.cap * 2 * M_PI / 360);
    sinus = sin(dep.cap * 2 * M_PI / 360);

    //newPOS = oldPOS + Vt
    dep_x = cosinus * dep.vitesse * 10 / VITMIN;
    dep_y = sinus * dep.vitesse * 10 / VITMIN;

    // on se deplace d'au moins une case quels que soient le cap et la vitesse
    // sauf si cap est un des angles droit
    if ((dep_x > 0) && (dep_x < 1)) dep_x = 1;
    if ((dep_x < 0) && (dep_x > -1)) dep_x = -1;

    if ((dep_y > 0) && (dep_y < 1)) dep_y = 1;
    if ((dep_y < 0) && (dep_y > -1)) dep_y = -1;

    //printf(" x : %f y : %f\n", dep_x, dep_y);

    coord.x = coord.x + (int) dep_x;
    coord.y = coord.y + (int) dep_y;

    //afficher_donnees();
}

// fonction principale : gere l'execution de l'avion au fil du temps

void se_deplacer() {
    while (1) {
        
        calcul_deplacement();
        envoyer_caracteristiques();
        sleep(4);
    }
}

int main(int argc, char const *argv[])
{
    
    initialiser_avion();
    char *val = (char*)malloc(sizeof(char)*5);
    //afficher_donnees();
    // on quitte si on arrive à pas contacter le gestionnaire de vols
    if (!ouvrir_communication()) {
        printf("Impossible de contacter le gestionnaire de vols\n");
        exit(1);
    }

    // on se déplace une fois toutes les initialisations faites
    pthread_t depalcerAvion;
    pthread_create(&depalcerAvion, NULL, se_deplacer, NULL);
    
    while(1)
    {
      lire  = read(sock, command, 1024);
      if(lire > 0)
      {
        memmove(val, command+2, strlen(command));
        
        if(command[0] == 'a') 
        {
          changer_altitude(atoi(val));
        }
        else if(command[0] == 'c')
        {
          changer_cap(atoi(val)); 
        }
        else//v
        {
          changer_vitesse(atoi(val));
        }
      }
    }

    
    return 0;
}

