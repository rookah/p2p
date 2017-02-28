#include "headers/p2p.h"
#include "headers/socklib.h"

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
    infos->port = "8888";
    infos->is_seed = 1;
    infos->seed_ip = "127.0.0.1";
    infos->download_dir = "~/";
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

void demandeTableVoisins(Infos_Locales infos, IP next) {
    int s = CreeSocketClient(next, PORT_SEED);
    char mess[TAILLE_ENTETE];
    sprintf(mess, FORMAT_ENTETE, 0, 1, 2, 3, 4, 19, 0, 1234567);
    printf("%s\n", mess);
    EnvoieMessage(s, mess);
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
            int more, ip[4], TTl, type;
            unsigned taille;
            if (recv(s, b_entete, TAILLE_ENTETE, MSG_WAITALL) == -1)
                perror("recv");
            sscanf(b_entete, FORMAT_ENTETE, &more, ip, ip+1, ip+2, ip+3, &TTl, &type, &taille);
        }
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
    if (infos.is_seed) {
        infos.port = PORT_SEED;
        printf("Démarrage en mode noeud racine...\n");

    } else {
        demandeTableVoisins(infos, infos.seed_ip);
        
    }

    int sock_attente = CreeSocketServeur(infos.port);
    if (sock_attente == -1)
        exit(EXIT_FAILURE);

    pid_t cpid = -1;

    /* Boucle du programme */
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
