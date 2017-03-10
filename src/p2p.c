#include "headers/p2p.h"
#include "headers/socklib.h"

#include <netdb.h>
#include <errno.h>

const char* program_name;


void afficheUsage(FILE* stream, const int exit_code)
{
    fprintf(stream, "Usage:  %s [options]\n", program_name);
    fprintf(stream,
            "  -h   --help          Affiche ce message d'aide.\n"
            "  -s   --server        @IP du premier noeud contacté. (défaut: 127.0.0.1)\n"
            "  -p   --server-port   N° du port du premier noeud contacté. (défaut: 8888)\n"
            "  -P   --port          N° du port utilisé. (défaut: 8889)\n"
            "  -d   --directory     Répertoire racine de partage des fichiers. (défaut: ~/)\n");
    exit(exit_code);
}

void afficheAide(FILE* stream)
{
    fprintf(stream,
            "   d   Affiche le répertoire de téléchargement\n"
            "   v   Affiche la table de voisins\n"
            "   +   Ajoute 5 secondes à l'intervalle du heartbeat (max. 2min)\n"
            "   -   Enlève 5 secondes de l'intervalle du heartbeat (min. 5sec)\n"
            "   ?   Affiche cette aide"
            "   q   Quitte le programme");
}

void initInfosLocales(Infos_Locales* infos)
{
    infos->local_ip = infos->seed_ip;
    infos->local_port = "8889";
    infos->seed_ip = "127.0.0.1";
    infos->seed_port = "8888";
    infos->is_seed = 1;
    infos->download_dir = "~/";
    infos->nb_voisins = 0;
    int i, j;
    for (i = 0; i < MAX_VOISINS; i++) {
        for (j = 0; j < 4; j++)
            infos->tab_voisins[i].ip[j] = 0;
        infos->tab_voisins[i].port = 0;
    }
}

void creeDir(const char* dir)
{
    pid_t cpid = fork();
    if (cpid == -1) {
        perror("Fork");
        exit(EXIT_FAILURE);

    } else if (cpid == 0) {
        char tmp[strlen(dir)]; 
        strcpy(tmp, dir);
        char* args[4] = {"mkdir", "-p", tmp, NULL};
        execvp("mkdir", args);
        perror("mkdir");
        exit(EXIT_FAILURE);

    } else {
        int wstatus;
        waitpid(-1, &wstatus, 0);
        if (WEXITSTATUS(wstatus) == EXIT_FAILURE)
            exit(EXIT_FAILURE);
    }
}

void checkPort(const char* port)
{
    unsigned n_port;
    if (sscanf(port, "%u", &n_port) != 1) {
        fprintf(stderr, "Erreur lecture port \"%s\": Mauvais format", port);
        exit(EXIT_FAILURE);

    } else if (n_port <= 1024 || n_port > 65535) {
        fprintf(stderr, "Erreur lecture port \"%s\": Mauvaise valeur", port);
        exit(EXIT_FAILURE);
    }
}

char* getIp(const int socket)
{
    int res;
    struct addrinfo info;
    socklen_t tailleinfo = sizeof(info);
    char hname[NI_MAXHOST], sname[NI_MAXSERV];

    res = getsockname(socket, (struct sockaddr *)&info, &tailleinfo);
    if (res == -1) {
        perror("getsockname");
        exit(1);
    }
    res = getnameinfo((struct sockaddr *)&info, tailleinfo,
            hname, NI_MAXHOST,
            sname, NI_MAXSERV,
            NI_NUMERICSERV|NI_NUMERICHOST);
    if (res != 0) {
        fprintf(stdout, "getnameinfo: %s\n", gai_strerror(res));
        exit(1);
    }

    char* ret = (char*) malloc(sizeof(hname));
    ret = hname;
    return ret;
}

void transformeIp(const char* ip, int tab[4])
{
    sscanf(ip, "%d.%d.%d.%d", &tab[0], &tab[1], &tab[2], &tab[3]);
}

void demandeTableVoisins(Infos_Locales* infos, const char* addr, const char* port) {
    if (port == infos->local_port)
        return;
    int s = CreeSocketClient(addr, port);
    if (s == -1) {
        fprintf(stderr, "warning: %s déconnecté du réseau\n", addr);
        return;
    }

    infos->local_ip = getIp(s);
    int ip[4];
    transformeIp(infos->local_ip, ip);

    EnvoieMessage(s, FORMAT_ENTETE, 0, ip[0], ip[1], ip[2], ip[3], 0, 1, 0);


    /* On reçoit la table de voisins potentiels */
    Infos_Pair tab_voisins_potentiels[MAX_VOISINS+1];
    int si;
    if ((si = recv(s, tab_voisins_potentiels, sizeof (tab_voisins_potentiels), MSG_WAITALL)) == -1)
        perror("recv");

    int i;
    for (i = 0; i < MAX_VOISINS + 1; i++) {
        if (infos->nb_voisins == MAX_VOISINS)
            break;
        if (tab_voisins_potentiels[i].port != 0) {
            demandeVoisin(infos, tab_voisins_potentiels[i]);
        }
    }

    if (infos->nb_voisins == 0) { //On recommence avec les entrées de la table de voisins si on n'a pas de voisin
        char addr2[16], port2[6];
        for (i = 0; i < MAX_VOISINS; i++) {
            if (tab_voisins_potentiels[i].port != 0) {
                sprintf(addr2, "%d.%d.%d.%d", tab_voisins_potentiels[i].ip[0], tab_voisins_potentiels[i].ip[0],
                        tab_voisins_potentiels[i].ip[2], tab_voisins_potentiels[i].ip[3]);
                sprintf(port2, "%d", tab_voisins_potentiels[i].port);
                demandeTableVoisins(infos, addr2, port2);
                if (infos->nb_voisins != 0)
                    break;
            }
        }
    }
    close(s);
}

void demandeVoisin(Infos_Locales* infos, Infos_Pair node)
{
    if (node.port == atoi(infos->local_port))
        return;
    char port[6];
    sprintf(port, "%d", node.port);

    char addr[16];
    sprintf(addr, "%d.%d.%d.%d", node.ip[0], node.ip[1], node.ip[2], node.ip[3]);

    int s = CreeSocketClient(addr, port);
    if (s == -1)
        return;

    infos->local_ip = getIp(s);
    int ip[4];
    transformeIp(infos->local_ip, ip);

    EnvoieMessage(s, FORMAT_ENTETE, 0, ip[0], ip[1], ip[2], ip[3], 0, 2, 6);

    char accept[2];
    int r = recv(s, accept, sizeof(char), MSG_WAITALL);
    if (r == -1) {
        perror("recv");
        exit(EXIT_FAILURE);
    }

    if (atoi(accept)) {
        int port_int;
        sscanf(infos->local_port, "%d", &port_int);
        EnvoieMessage(s, "%d", port_int);
        int i, j;
        for (i = 0; i < MAX_VOISINS; i++) {
            if (infos->tab_voisins[i].port == 0) {
                for (j = 0; j < 4; j++)
                    infos->tab_voisins[i].ip[j] = node.ip[j];
                infos->tab_voisins[i].port = node.port;
                infos->nb_voisins++;
                printf("Nouveau voisin! @IP: %d.%d.%d.%d\n", infos->tab_voisins[i].ip[0],
                        infos->tab_voisins[i].ip[1], infos->tab_voisins[i].ip[2], infos->tab_voisins[i].ip[3]);
                break;
            }
        }
    }
    close(s);
}

void demandeHeartbeat(Infos_Locales* infos)
{
    int i, s;
    char port[6];
    char addr[16];
    Infos_Pair node;
    for (i = 0; i < MAX_VOISINS; i++) {
        if (infos->tab_voisins[i].port != 0) {
            node = infos->tab_voisins[i];
            sprintf(port, "%d", node.port);
            sprintf(addr, "%d.%d.%d.%d", node.ip[0], node.ip[1], node.ip[2], node.ip[3]);

            if ((s = CreeSocketClient(addr, port)) == -1) { //On supprime le voisin car il n'est pas joinable
                infos->tab_voisins[i].port = 0;
                infos->nb_voisins--;
                printf("Voisin %d.%d.%d.%d/%d déconnecté\n", node.ip[0], node.ip[1], node.ip[2], node.ip[3], node.port);

            } else {
                close(s);
            }
        }
    }
}

void demandeDeconnexion(const Infos_Locales infos) {
    int i, s;
    char port[6];
    char addr[16];
    pid_t cpid;
    for (i = 0; i < MAX_VOISINS; i++) {
        if (infos.tab_voisins[i].port != 0) {
            cpid = fork();
            if (cpid == -1) {
                perror("fork");
                exit(EXIT_FAILURE);

            } else if (cpid == 0) {
                Infos_Pair node = infos.tab_voisins[i];
                sprintf(port, "%d", node.port);
                sprintf(addr, "%d.%d.%d.%d", node.ip[0], node.ip[1], node.ip[2], node.ip[3]);
                if ((s = CreeSocketClient(addr, port)) == -1) {
                    fprintf(stderr, "warning: %s déconnecté du réseau\n", addr);
                    continue;
                }

                exit(EXIT_SUCCESS);
            }
        }
    }
}

void traitementServeur(Infos_Locales* infos, int sock_attente)
{
    int s;
    // On ne fork pas pour conserver les infos locales modifiées
    while ((s = TryAcceptConnexion(sock_attente)) != 0) {
        if (s == -1) {
            perror("socket");
            exit(EXIT_FAILURE);
        }
        char b_entete[TAILLE_ENTETE];
        int more, ip[4], TTL, type;
        unsigned taille;
        if (recv(s, b_entete, TAILLE_ENTETE, MSG_WAITALL) == -1) {
            perror("recv");
            continue;
        }
        sscanf(b_entete, FORMAT_ENTETE, &more, ip, ip+1, ip+2, ip+3, &TTL, &type, &taille);

        switch(type) {
            case -1:
                deleteVoisin(infos, ip, s);
                break;
            case 0:
                break;
            case 1: //demande de table de voisins
                traiteDemandeTableVoisins(*infos, s);
                break;
            case 2: //traitement de voisin
                traiteDemandeVoisin(infos, ip, s);
                break;
            case 3:
                break;
            case 4:
                break;
            default:
                break;
        }
        close(s);
    }
}

void traiteDemandeTableVoisins(const Infos_Locales infos, int socket)
{
    Infos_Pair tab_voisins_potentiels[MAX_VOISINS + 1];
    memcpy(tab_voisins_potentiels, infos.tab_voisins, sizeof(Infos_Pair) * MAX_VOISINS);

    if (infos.nb_voisins < MAX_VOISINS) { //On s'ajoute à la liste des voisins potentiels s'il nous manque des voisins
        sscanf(infos.local_port, "%d", &tab_voisins_potentiels[MAX_VOISINS].port);

        /* On donne l'adresse IP utilisée pour la socket */
        char* local_ip_socket = infos.local_ip;
        local_ip_socket = getIp(socket);
        sscanf(local_ip_socket, "%d.%d.%d.%d", &tab_voisins_potentiels[MAX_VOISINS].ip[0], &tab_voisins_potentiels[MAX_VOISINS].ip[1],
                &tab_voisins_potentiels[MAX_VOISINS].ip[2], &tab_voisins_potentiels[MAX_VOISINS].ip[3]);

    } else {
        tab_voisins_potentiels[MAX_VOISINS].port = 0;
    }

    /* Envoi de la table */
    if (write(socket, tab_voisins_potentiels, sizeof(tab_voisins_potentiels)) == -1) {
        perror("write");
    }
}

void traiteDemandeVoisin(Infos_Locales* infos, int ip[4], int socket)
{
    if (infos->nb_voisins < MAX_VOISINS) { //On ajoute le voisin à notre table, et on lui donne notre accord pour qu'il nous ajoute
        EnvoieMessage(socket, "1");
        char port_string[6];
        if (recv(socket, port_string, sizeof(port_string), MSG_WAITALL) == -1) {
            perror("recv");
            exit(EXIT_FAILURE);
        }
        int port = atoi(port_string);
        int i, j;
        for (i = 0; i < MAX_VOISINS; i++) {
            if (infos->tab_voisins[i].port == 0) {
                for (j = 0; j < 4; j++)
                    infos->tab_voisins[i].ip[j] = ip[j];
                infos->tab_voisins[i].port = port;
                infos->nb_voisins++;
                printf("Nouveau voisin! @IP: %d.%d.%d.%d\n", ip[0], ip[1], ip[2], ip[3]); 
                break;
            }
        }

    } else {
        EnvoieMessage(socket, "0");
    }
}

void deleteVoisin(Infos_Locales* infos, int ip[4], int socket)
{
        char port_string[6];
        if (recv(socket, port_string, sizeof(port_string), MSG_WAITALL) == -1) {
            perror("recv");
            exit(EXIT_FAILURE);
        }
        int port = atoi(port_string);
}

int main(int argc, char *argv[])
{
    Infos_Locales infos;
    initInfosLocales(&infos);
    program_name = argv[0];
    int option;
    const char* short_options = "hs:p:P:d:";
    const struct option long_options[] = {
        { "help", 0, NULL, 'h' },
        { "server", 1, NULL, 's' },
        { "server-port", 1, NULL, 's' },
        { "directory", 1, NULL, 'd' },
        { "port", 1, NULL, 'p' },
        { NULL, 0, NULL, 0 }
    };

    do {
        option = getopt_long(argc, argv, short_options, long_options, NULL);
        switch (option) {
            case 'h':
                afficheUsage(stdout, EXIT_SUCCESS);

            case 's':
                infos.seed_ip = optarg;
                infos.is_seed = 0;
                break;

            case 'p':
                checkPort(optarg);
                infos.seed_port = optarg;
                break;

            case 'P':
                checkPort(optarg);
                infos.local_port = optarg;
                break;

            case 'd':
                creeDir(optarg);
                infos.download_dir = optarg;
                break;

            case '?': // Mauvaise option
                afficheUsage(stderr, EXIT_FAILURE);

            case -1: // Fin du parsing
                break;

            default:
                abort();
        }
    } while (option != -1);

    /* Initialisations client/serveur */
    if (infos.is_seed) {
        printf("Démarrage en mode noeud racine...\n");
        infos.local_port = infos.seed_port;

    } else {
        demandeTableVoisins(&infos, infos.seed_ip, infos.seed_port);
    }

    int sock_attente = CreeSocketServeur(infos.local_port);
    if (sock_attente == -1)
        exit(EXIT_FAILURE);


    /* Boucle du programme */
    int compteur = 0;
    int t = 20;
    int menu_principal = 1;
    while (1) {
        compteur = (compteur + 1) % t;
        if (!compteur)
            demandeHeartbeat(&infos);
        if (!infos.is_seed && infos.nb_voisins == 0)
            demandeTableVoisins(&infos, infos.seed_ip, infos.seed_port);
        traitementServeur(&infos, sock_attente);
        if (menu_principal) {
            printf("Menu principal\n");
            menu_principal = 0;
        }
        char buffer[256];
        buffer[0] = '\0';

        if (fgets_nonbloquant(buffer, sizeof buffer, stdin) != NULL) {
            menu_principal = 1;
            switch(buffer[0]) {
                case 'd':
                    printf("Répertoire partagé: %s\n", infos.download_dir);
                    break;

                case 'v':
                    printf("Nombre de voisins: %d\n", infos.nb_voisins);
                    int i, j = 1;
                    for (i = 0; i < MAX_VOISINS; i++) {
                        if (infos.tab_voisins[i].port != 0)
                            printf("Voisin %d: %d.%d.%d.%d/%d\n", j++, infos.tab_voisins[i].ip[0], infos.tab_voisins[i].ip[1], infos.tab_voisins[i].ip[2], infos.tab_voisins[i].ip[3], infos.tab_voisins[i].port);
                    }
                    break;
                case '+':
                    t += 10;
                    break;

                case '-':
                    if (t > 10)
                        t -= 10;
                    break;

                case '\n':
                    break;

                case '?':
                    afficheAide(stdout);
                    break;

                case 'q':
                    exit(EXIT_SUCCESS);

                default:
                    fprintf(stderr, "Erreur lecture input\n");
                    afficheAide(stderr);
                    break;
            }
        }
        usleep(500000);
    }

    return 0;
}
