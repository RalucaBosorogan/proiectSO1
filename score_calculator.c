#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#define TREASURE_FILE "Treasure"

#define MAX_PATH 512
#define MAX_TEASURE_ID 32
#define MAX_USER 64
#define MAX_CLUE 256

#define SCORE_CALCULATOR_FAIELD 1

typedef struct   
{
    float latitude;
    float longitude;
} GPS;

typedef struct  
{
    char treasure_id[MAX_TEASURE_ID];
    char user[MAX_USER];
    GPS coord;
    char clue[MAX_CLUE];
    int value;
} Treasure;

int huntScore = 0;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <hunt_id>\n", argv[0]);
        return 1;
    }

    char path[256];
    snprintf(path, sizeof(path), "%s/%s", argv[1], TREASURE_FILE);

    struct stat st;

    if (stat(path, &st) == -1)
    {
        perror("Treasure file!");
        exit(SCORE_CALCULATOR_FAIELD);
    }


    int fd = open(path, O_RDONLY);
    if (!fd) {
        perror("Failed to open treasure file");
        return 1;
    }

    Treasure t;

    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure))
    {
        huntScore += t.value;
        // printf("- ID: %s, User: %s, Location: (%.2f, %.2f), Value: %d\n",
        //        t.treasure_id, t.user, t.coord.latitude, t.coord.longitude, t.value);
    }

    printf("The score of the hunt \"%s\" is %d!\n",argv[1],huntScore);
    return 0;
}
