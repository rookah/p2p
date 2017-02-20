#include "headers/p2p.h"
#include "headers/socklib.h"


const char* program_name;
const char* download_dir = "~/";


void afficheUsage(FILE* stream, const int exit_code)
{
    fprintf(stream, "Usage:  %s [options]\n", program_name);
    fprintf(stream,
            "  -h   --help      Affiche ce message d'aide.\n"
            "  -p   --peer      @IP du premier noeud contacté.\n"
            "  -d   --directory Répertoire racine de partage des fichiers.\n");
    exit(exit_code);
}

void parseIp(const char* str, IP* ip)
{
    if (sscanf(str, "%d.%d.%d.%d", &(*ip)[0], &(*ip)[1], &(*ip)[2], &(*ip)[3]) != 4) {
        fprintf(stderr, "Erreur lecture @IP \"%s\": Mauvais format", str);
        exit(EXIT_FAILURE);
    }

    int i;
    for (i = 0; i < 4; i++)
        if ((*ip)[i] < 0 || (*ip)[i] > 255) {
            fprintf(stderr, "Erreur lecture @IP \"%s\": Mauvaise(s) valeur(s)", str);
            exit(EXIT_FAILURE);
        }
}

void creeDir(char* dir)
{
    pid_t cpid = fork();
    if (cpid == -1) {
        perror("Fork");
        exit(EXIT_FAILURE);

    } else if (cpid == 0) {
        char* args[4] = {"mkdir", "-p", dir, NULL};
        execvp("mkdir", args);
        perror("mkdir");
        exit(EXIT_FAILURE);

    } else {
        waitpid(-1, NULL, 0);
    }
}

int main(int argc, char *argv[])
{
    program_name = argv[0];
    int option;
    const char* const short_options = "hp:d:";
    const struct option long_options[] = {
        { "help", 0, NULL, 'h' },
        { "peer", 1, NULL, 'p' },
        { "directory", 1, NULL, 'd' },
        { NULL, 0, NULL, 0 }
    };

    IP ip;
    do {
        option = getopt_long(argc, argv, short_options, long_options, NULL);
        switch (option) {
            case 'h':
                afficheUsage(stdout, EXIT_SUCCESS);

            case 'p':
                parseIp(optarg, &ip);
                break;

            case 'd':
                creeDir(optarg);
                download_dir = optarg;
                break;

            case '?': // Mauvaise option
                afficheUsage(stderr, EXIT_FAILURE);

            case -1: // Fin du parsing
                break;

            default:
                abort();
        }
    } while (option != -1);

    return 0;
}
