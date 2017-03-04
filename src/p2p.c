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
            "  -d   --directory     Répertoire racine de partage des fichiers. (défaut: ~/)\n"
            "  -c   --port          N° du port utilisé. (défaut: 8888)\n");
    exit(exit_code);
}

void afficheAide(FILE* stream)
{
    fprintf(stream, "AIDE ICI～\n");
}

void initInfosLocales(Infos_Locales* infos)
{
    infos->port = "8889";
    infos->is_seed = 1;
    infos->seed_ip = "127.0.0.1";
    infos->download_dir = "~/";
    infos->nb_voisins = 0;
    int i;
    for (i = 0; i < MAX_VOISINS; i++) {
        infos->tab_voisins[i].ip[0] = 0;
        infos->tab_voisins[i].ip[1] = 0;
        infos->tab_voisins[i].ip[2] = 0;
        infos->tab_voisins[i].ip[3] = 0;
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

void getIp(const int socket, char** ip)
{
    int res;
    struct addrinfo info;
    socklen_t tailleinfo = sizeof(info);
    char hname[NI_MAXHOST], sname[NI_MAXSERV];

    // pour avoir sa propre IP a partir d'une socket
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

    *ip = hname;
}

void transformeIp(const char* ip, int tab[4])
{
    sscanf(ip, "%d.%d.%d.%d", &tab[0], &tab[1], &tab[2], &tab[3]);
}

void demandeTableVoisins(Infos_Locales* infos, IP addr, PORT port) {
    int s = CreeSocketClient(addr, port);
    if (s == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    getIp(s, &infos->local_ip);
    int ip[4];
    transformeIp(infos->local_ip, ip);

    char mess[TAILLE_ENTETE];
    sprintf(mess, FORMAT_ENTETE, 0, ip[0], ip[1], ip[2], ip[3], 0, 1, 0);
    EnvoieMessage(s, mess);

    /* On reçoit la table de voisins potentiels */
    Infos_Pair tab_voisins_potentiels[MAX_VOISINS+1];
    if (recv(s, tab_voisins_potentiels, sizeof (Infos_Pair) * (MAX_VOISINS + 1), MSG_WAITALL) == -1)
        perror("recv");

    /* Si le noeud qu'on a contacté s'est ajouté */
    if (tab_voisins_potentiels[MAX_VOISINS].port != 0) {
        demandeVoisin(infos, tab_voisins_potentiels[MAX_VOISINS]);
    }

    int i;
    for (i = 0; i < MAX_VOISINS; i++) {
        if (infos->nb_voisins == MAX_VOISINS)
            break;
        if (tab_voisins_potentiels[i].port != 0)
            demandeVoisin(infos, tab_voisins_potentiels[i]);
    }
}

void demandeVoisin(Infos_Locales* infos, Infos_Pair node)
{
    char port[6];
    char addr[16];
    sprintf(port, "%d", node.port);
    sprintf(addr, "%d.%d.%d.%d", node.ip[0], node.ip[1], node.ip[2], node.ip[3]);
    int s = CreeSocketClient(addr, port);
    if (s == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    char mess[TAILLE_ENTETE];
    sprintf(mess, FORMAT_ENTETE, 0, node.ip[0], node.ip[1], node.ip[2], node.ip[3], 0, 2, 6);
    EnvoieMessage(s, mess);
    EnvoieMessage(s, port);

    int accept;
    int r = recv(s, &accept, 1, MSG_WAITALL);
    if (r == -1) {
        perror("recv");
        exit(EXIT_FAILURE);
    }

    if (accept) {
        Infos_Pair new;
        r = recv(s, &new, sizeof(Infos_Pair), MSG_WAITALL);
        if (r == -1) {
            perror("recv");
            exit(EXIT_FAILURE);
        }
        int i, j;
        for (i = 0; i < MAX_VOISINS; i++) {
            if (infos->tab_voisins[i].port == 0) {
                for (j = 0; j < 4; j++)
                    infos->tab_voisins[i].ip[j] = new.ip[j];
                infos->tab_voisins[i].port = new.port;
                infos->nb_voisins++;
                break;
            }
        }
    }
}

void traitementServeur(Infos_Locales* infos, int sock_attente)
{
    int s;
    pid_t cpid;
    while ((s = TryAcceptConnexion(sock_attente)) != 0) {
        cpid = fork();
        if (cpid == -1) {
            perror("fork");
            exit(EXIT_FAILURE);

        } else if (cpid == 0) {
            /* Dans notre cas, le serveur reçoit toujours un message avant d'en envoyer un */
            char b_entete[TAILLE_ENTETE];
            int more, ip[4], TTL, type;
            unsigned taille;
            if (recv(s, b_entete, TAILLE_ENTETE, MSG_WAITALL) == -1)
                perror("recv");
            sscanf(b_entete, FORMAT_ENTETE, &more, ip, ip+1, ip+2, ip+3, &TTL, &type, &taille);

            switch(type) {
                case -1:
                    break;
                case 0: //heartbeat
                    break;
                case 1: //demande de table de voisins
                    traiteDemandeTableVoisins(*infos, s);
                    break;
                case 2:
                    break;
                case 3:
                    break;
                default:
                    fprintf(stderr, "Type de demande inconnu: %d", type);
                    exit(EXIT_FAILURE);
            }
            exit(EXIT_SUCCESS);
        }
    }
}

void traiteDemandeTableVoisins(const Infos_Locales infos, int socket)
{
    Infos_Pair tab_voisins_potentiels[MAX_VOISINS + 1];
    memcpy(tab_voisins_potentiels, infos.tab_voisins, sizeof(Infos_Pair) * MAX_VOISINS);

    if (infos.nb_voisins < MAX_VOISINS) { //On s'ajoute à la liste des voisins potentiels si il nous manque des voisins
        sscanf(infos.port, "%d", &tab_voisins_potentiels[MAX_VOISINS].port);
        sscanf(infos.local_ip, "%d.%d.%d.%d", &tab_voisins_potentiels[MAX_VOISINS].ip[0], &tab_voisins_potentiels[MAX_VOISINS].ip[1],
                &tab_voisins_potentiels[MAX_VOISINS].ip[2], &tab_voisins_potentiels[MAX_VOISINS].ip[3]);

    } else {
        tab_voisins_potentiels[MAX_VOISINS].port = 0;
    }

    /* Envoi de la table */
    if (write(socket, tab_voisins_potentiels, sizeof(Infos_Pair) * (MAX_VOISINS + 1)) == -1) {
        perror("write");
    }
}


int main(int argc, char *argv[])
{
    Infos_Locales infos;
    initInfosLocales(&infos);
    program_name = argv[0];
    int option;
    const char* short_options = "hs:d:p:";
    const struct option long_options[] = {
        { "help", 0, NULL, 'h' },
        { "server", 1, NULL, 's' },
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

            case 'd':
                creeDir(optarg);
                infos.download_dir = optarg;
                break;

            case 'p':
                checkPort(optarg);
                infos.port = optarg;
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
    int sock_attente = CreeSocketServeur(infos.port);
    if (sock_attente == -1)
        exit(EXIT_FAILURE);

    if (infos.is_seed) {
        infos.port = PORT_SEED;
        printf("Démarrage en mode noeud racine...\n");

    } else {
        printf("Demande de voisins en cours...\n");
        demandeTableVoisins(&infos, infos.seed_ip, PORT_SEED);
    }

    /* Boucle du programme */
    pid_t cpid = -1;
    while (1) {
        traitementServeur(&infos, sock_attente);
        char buffer[256];

        if (cpid == -1 || waitpid(cpid, NULL, WNOHANG) > 0) {
            cpid = fork();
            if (cpid == -1) {
                perror("fork");
                exit(EXIT_FAILURE);

            } else if (cpid == 0) {
                printf("Menu principal\n");
                while (fgets(buffer, sizeof buffer, stdin) == NULL)
                    fprintf(stderr, "Erreur lecture input (? pour aide)\n");

                switch(buffer[0]) {
                    case '1':
                        printf("%s\n", infos.download_dir);
                        break;
                    case '?':
                        afficheAide(stdout);
                        break;
                    default:
                        fprintf(stderr, "Erreur lecture input\n");
                        afficheAide(stderr);
                        break;
                }
                exit(EXIT_SUCCESS);
            }

            system("sleep 0.2");
        }
    }

    return 0;
}
