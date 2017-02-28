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
#define PORT_SEED "8889"
#define MAX_VOISINS 5
#define TAILLE_NOM_FICHIER 100
#define TAILLE_ENTETE 39
#define FORMAT_ENTETE "%1d %3d %3d %3d %3d %2d %1d %16u"


/** Définitions de types */
typedef char* IP;
typedef char* PORT;

typedef struct Infos_Pair {
    IP ip;
    PORT port;
} Infos_Pair;

typedef struct Infos_Locales {
    char* download_dir;
    int is_seed;
    PORT port;
    IP seed_ip;
    Infos_Pair tab_voisins[MAX_VOISINS];
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
void demandeTableVoisins(Infos_Locales infos, IP next); 

/** Accepte les connexions entrantes et crée des forks pour les traîter
  * infos:  informations locales
  * sock_attente:      socket d'écoute
  */
void traitementServeur(Infos_Locales* infos, int sock_attente);

#endif /* P2P_H */
