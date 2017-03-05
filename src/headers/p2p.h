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

typedef struct Infos_Pair {
    int ip[4];
    int port;
} Infos_Pair;

typedef struct Infos_Locales {
    char* download_dir;
    int is_seed;
    char* port;
    char* local_ip;
    char* seed_ip;
    Infos_Pair tab_voisins[MAX_VOISINS];
    int nb_voisins;
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

/** Récupère l'IP de la socket passée en paramètre
 * socket:   socket
 * ip:       string où on stocke l'@IP
 */
void getIp(const int socket, char** ip);

/** Transforme une IP en int
 * ip:     @IP
 * tab:    tableau de int
 */
void transformeIp(const char* ip, int tab[4]);

/** Demande une table de voisins à contacter pour devenir nouveau voisin au noeud central.
 * infos:  pointeur sur les informations locales
 * addr:   @IP à contacter
 * port:   port associé
 */
void demandeTableVoisins(Infos_Locales* infos, const char* addr, const char* port);

/** Demande à une node de devenir notre voisin
 * infos:   pointeur sur les informations locales
 * node:    informations du voisin
 * socket:  socket déjà ouverte, ou 0 s'il n'y en a pas
 */
void demandeVoisin(Infos_Locales* infos, const Infos_Pair node, int s);

/** Demande aux voisins de répondre à un message pour vérifier leur existence
 * infos:   pointeur sur les informations locales
 */
void demandeHeartbeat(Infos_Locales* infos);

/** Accepte les connexions entrantes et crée des forks pour les traîter
 * infos:           pointeur sur les informations localesinformations locales
 * sock_attente:    socket d'écoute
 */
void traitementServeur(Infos_Locales* infos, const int sock_attente);

/** Traite une demande de table de voisins
 * infos:           informations locales
 * socket: socket
 */
void traiteDemandeTableVoisins(const Infos_Locales infos, int socket);

/** Traite une demande de voisin
 * infos:   informations locales
 * ip:      @IP
 * socket:  socket
 */
void traiteDemandeVoisin(Infos_Locales* infos, int ip[4], int socket);

/** Répond à un heartbeat
 * infos:   informations locales
 */
void traiteHeartbeat(int socket);

#endif /* P2P_H */
