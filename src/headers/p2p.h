#ifndef P2P_H
#define P2P_H

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>


/** Définitions statiques */
#define MAX_VOISINS 5
#define PORT 8888


/** Définitions de types */
typedef int IP[4];

typedef struct Client_Local {
    IP self_ip;
    IP peers[MAX_VOISINS];
} Client_Local;


/** Fonctions */

/** Affiche l'usage (aide) et termine le programme
  * stream:     stream dans lequel le message sera affiché
  * exit_code:  code de terminaison
  */ 
void afficheUsage(FILE* stream, const int exit_code);

/** Parse une adresse IP à partir d'une string ; termine le programme si mauvaise IP
  * str:    String contenant @IP
  * IP:     Reçoit les 4 octets de l'adresse
  */
void parseIp(const char* str, IP* ip);

/** Crée récursivement un dossier ; termine le programme si erreur
  * dir:    String contenant le nom du dossier
  */
void creeDir(const char* dir);

/* Affiche */

#endif /* P2P_H */
