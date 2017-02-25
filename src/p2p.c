#include "headers/p2p.h"
#include "headers/socklib.h"


const char* program_name;


void afficheUsage(FILE* stream, const int exit_code)
{
    fprintf(stream, "Usage:  %s [options]\n", program_name);
    fprintf(stream,
            "  -h   --help          Affiche ce message d'aide.\n"
            "  -a   --address       @IP du premier noeud contacté. (défaut: 127.0.0.1)\n"
            "  -d   --directory     Répertoire racine de partage des fichiers. (défaut: ~/)\n"
            "  -c   --client-port   N° du port client. (défaut: 8888)\n"
            "  -s   --server-port   N° du port serveur. (défaut: 8889)\n");
    exit(exit_code);
}

void afficheAide(FILE* stream)
{
    fprintf(stream, "AIDE ICI～\n");
}

void initInfosLocales(Infos_Locales* infos)
{
    infos->port_client = "8888";
    infos->port_serveur = "8889";
    infos->seed_ip[0] = 127; infos->seed_ip[1] = 0; infos->seed_ip[2] = 0; infos->seed_ip[3] = 1;
    infos->download_dir = "~/";
}

void parseIp(const char* str, Infos_Locales* infos)
{
    int ip[4];
    if (sscanf(str, "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]) != 4) {
        fprintf(stderr, "Erreur lecture @IP \"%s\": Mauvais format", str);
        exit(EXIT_FAILURE);
    }

    int i;
    for (i = 0; i < 4; i++) {
        if (ip[i] < 0 || ip[i] > 255) {
            fprintf(stderr, "Erreur lecture @IP \"%s\": Mauvaise(s) valeur(s)", str);
            exit(EXIT_FAILURE);
        }
        infos->seed_ip[i] = (unsigned char) ip[i];
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

void boucleServeur(Infos_Locales* infos)
{
    /* On fork pour recevoir les requêtes en parallèle */
    pid_t cpid = fork();
    if (cpid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);

    } else if (cpid == 0) {
        int sock_attente = CreeSocketServeur(infos->port_serveur);
        if (sock_attente == -1)
            exit(EXIT_FAILURE);
        char buff[23];

        while(1) {
            int s = AcceptConnexion(sock_attente);
            /* Dans notre cas, le serveur reçoit d'abord un message avant d'envoyer */
            int r = recv(s, buff, 29, MSG_WAITALL);
            if (r == -1)
                perror("recv");
            char more, TTL, type;
            IP ip;
            sscanf(buff, "%c %hhu %hhu %hhu %hhu %c %c", &more, &ip[0], &ip[1], &ip[2], &ip[3], &TTL, &type);
            printf("%hhu\n", ip[3]);
        }
    }
}


int main(int argc, char *argv[])
{
    Infos_Locales infos;
    initInfosLocales(&infos);
    program_name = argv[0];
    int option;
    const char* const short_options = "ha:d:c:s:";
    const struct option long_options[] = {
        { "help", 0, NULL, 'h' },
        { "address", 1, NULL, 'a' },
        { "directory", 1, NULL, 'd' },
        { "client-port", 1, NULL, 'c' },
        { "server-port", 1, NULL, 's' },
        { NULL, 0, NULL, 0 }
    };

    do {
        option = getopt_long(argc, argv, short_options, long_options, NULL);
        switch (option) {
            case 'h':
                afficheUsage(stdout, EXIT_SUCCESS);

            case 'a':
                parseIp(optarg, &infos);
                infos.is_seed = 0;
                break;

            case 'd':
                creeDir(optarg);
                infos.download_dir = optarg;
                break;

            case 'c':
                checkPort(optarg);
                infos.port_client = optarg;
                break;

            case 's':
                checkPort(optarg);
                infos.port_serveur = optarg;
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

    } else {
        ;
    }

    boucleServeur(&infos);

    int boucle_menu = 1;
    char buffer[256];
    while (boucle_menu) {
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
    }

    return 0;
}
