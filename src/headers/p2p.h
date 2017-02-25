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
#define TAILLE_NOM_FICHIER 100


/** Définitions de types */
typedef unsigned char IP[4];

typedef struct Infos_Locales {
    char* download_dir;
    char* port_serveur;
    char* port_client;
    int is_seed;
    IP seed_ip;
    IP tab_voisins[MAX_VOISINS];
} Infos_Locales;

/** Fonctions */

/** Affiche l'usage et termine le programme
  * stream:     stream dans lequel le message sera affiché
  * exit_code:  code de terminaison
  */ 
void afficheUsage(FILE* stream, const int exit_code);

/** Affiche l'aide du menu
  * stream:     stream dans lequel le message sera affiché
  */ 
void afficheAide(FILE* stream);

/** Initialise les informations locales avec les valeurs par défaut
  * infos:  structure à initialiser
  */
void initInfosLocales(Infos_Locales* infos);

/** Parse une adresse IP ; termine le programme si mauvaise IP
  * str:    string contenant @IP
  */
void parseIp(const char* str, Infos_Locales* infos);

/** Crée récursivement un dossier ; termine le programme si erreur
  * dir:    string contenant le nom du dossier
  */
void creeDir(const char* dir);

/** Vérifie que le port passé en paramètre est correct
  * port:   chaine de caractère contenant le port
  */
void checkPort(const char* port);

/** Rempli la table des voisins en demandant aux voisins présents dans notre propre table. Remplit la table locale au passage
  * tab_voisins:    pointeur sur tableau contenant les IP déjà présentes dans notre table
  */
void remplirTableVoisins(IP* tab_voisins[]); 

/** Crée la socket d'écoute et la boucle de traitement de requêtes
  * infos:  informations locales
  */
void boucleServeur(Infos_Locales* infos);

#endif /* P2P_H */
