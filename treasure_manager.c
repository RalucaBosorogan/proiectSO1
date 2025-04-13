#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>

#define TREASURE_FILE "Treasure"
#define LOG_FILE "treasure_manager_log"

#define TREASURE_SUCCESS 0
#define TREASURE_FAILURE 1

#define MAX_PATH 512
#define MAX_TEASURE_ID 32
#define MAX_USER 64
#define MAX_CLUE 256

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

typedef enum {ADD,LIST,VIEW,REMOVE_TREASURE,REMOVE_HUNT} treasureManagerCommand;

void log_operation(const char *hunt_id, const char *operation)
{
    char log_path[MAX_PATH];
    snprintf(log_path, MAX_PATH, "%s/%s", hunt_id, LOG_FILE);

    int fd = open(log_path, O_WRONLY | O_CREAT | O_APPEND, 0644);

    if (fd == -1)
    {
        perror("[ log_operation() ]: Error at open()");
        return;
    }

    time_t now = time(NULL);

    char *timestamp = ctime(&now);

    timestamp[strlen(timestamp) - 1] = '\0'; // remove newline

    char buffer[512];
    snprintf(buffer, sizeof(buffer), "[%s] %s\n", timestamp, operation);

    write(fd, buffer, strlen(buffer));
    close(fd);

    // Create symlink
    char symlink_name[MAX_PATH];
    snprintf(symlink_name, MAX_PATH, "logged_hunt-%s", hunt_id);
    symlink(log_path, symlink_name); 
}

void add_treasure(const char *hunt_id, const char *tid, const char *user, float lat, float lon, const char *clue, int value)
{
    char dir_path[MAX_PATH];
    snprintf(dir_path, MAX_PATH, "%s", hunt_id);

    mkdir(dir_path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

    char file_path[MAX_PATH];
    snprintf(file_path, MAX_PATH, "%s/%s", hunt_id, TREASURE_FILE);

    int fd = open(file_path, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd == -1)
    {
        perror("[ add_treasure(....) ]: Error at open()");
        exit(TREASURE_FAILURE);
    }

    Treasure t;

    strncpy(t.treasure_id, tid, sizeof(t.treasure_id));
    strncpy(t.user, user, sizeof(t.user));

    t.coord.latitude = lat;
    t.coord.longitude = lon;

    strncpy(t.clue, clue, sizeof(t.clue));
    
    t.value = value;

    write(fd, &t, sizeof(Treasure));
    close(fd);

    log_operation(hunt_id, "Added treasure");
    printf("Treasure added successfully.\n");
}

void list_treasures(const char *hunt_id)
{
    char file_path[MAX_PATH];
    snprintf(file_path, MAX_PATH, "%s/%s", hunt_id, TREASURE_FILE);

    struct stat st;

    if (stat(file_path, &st) == -1)
    {
        perror("[ list_treasures(...) ] Error at stat(...)");
        return;
    }

    printf("Hunt: %s\n", hunt_id);
    printf("Size: %ld bytes\n", st.st_size);
    printf("Last modified: %s", ctime(&st.st_mtime));

    int fd = open(file_path, O_RDONLY);

    if (fd == -1)
    {
        perror("[ list_treasures(...) ] Error at open(...) file not found");
        return;
    }

    Treasure t;

    lseek(fd,0,SEEK_SET);

    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure))
    {
        printf("- ID: %s, User: %s, Location: (%.2f, %.2f), Value: %d\n",
               t.treasure_id, t.user, t.coord.latitude, t.coord.longitude, t.value);
    }

    close(fd);
    log_operation(hunt_id, "Listed treasures");
}

void view_treasure(const char *hunt_id, const char *id)
{
    char file_path[MAX_PATH];
    snprintf(file_path, MAX_PATH, "%s/%s", hunt_id, TREASURE_FILE);

    int fd = open(file_path, O_RDONLY);
    if (fd == -1)
    {
        perror("[ view_treasure(...) ] Error at open(...) file not found");
        return;
    }

    Treasure t;
    
    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure))
    {
        if (strcmp(t.treasure_id, id) == 0)
        {
            printf("ID: %s\nUser: %s\nLat: %.2f\nLong: %.2f\nClue: %s\nValue: %d\n", t.treasure_id, t.user, t.coord.latitude, t.coord.longitude, t.clue, t.value);
            log_operation(hunt_id, "Viewed treasure");
            close(fd);
            return;
        }
    }

    printf("Treasure not found.\n");
    close(fd);
}

void remove_treasure(const char *hunt_id, const char *tid)
{

    char file_path[MAX_PATH];
    snprintf(file_path, MAX_PATH, "%s/%s", hunt_id, TREASURE_FILE);

    int fd = open(file_path, O_RDONLY);
    if (fd == -1)
    {
        perror("[ remove_treasure(...) ] Error at open(...) file not found");
        return;
    }

    char temp_path[MAX_PATH];
    snprintf(temp_path, MAX_PATH, "%s/temp", hunt_id);
    int tmp = open(temp_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    Treasure t;
    
    int found = 0;

    lseek(fd,0,SEEK_SET);

    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure))
    {
        if (strcmp(t.treasure_id, tid) != 0)
        {
            write(tmp, &t, sizeof(Treasure));
        }
        else
        {
            found = 1;
        }
    }
    close(fd);
    close(tmp);
    rename(temp_path, file_path);

    if (found)
    {
        printf("Treasure removed.\n");
        log_operation(hunt_id, "Removed treasure");
    }
    else
    {
        printf("Treasure not found.\n");
    }
}

void remove_hunt(const char *hunt_id)
{
    char file_path[MAX_PATH];
    char log_path[MAX_PATH];
    char symlink_name[MAX_PATH];

    snprintf(file_path, MAX_PATH, "%s/%s", hunt_id, TREASURE_FILE);
    unlink(file_path);

    snprintf(log_path, MAX_PATH, "%s/%s", hunt_id, LOG_FILE);
    unlink(log_path);
    
    snprintf(symlink_name, MAX_PATH, "logged_hunt-%s", hunt_id);
    unlink(symlink_name);

    rmdir(hunt_id);
    printf("Hunt '%s' removed.\n", hunt_id);
}

void print_help(const char *progName)
{
    printf("\n");
    printf("Usage: %s --<command> <hunt_id> [additional arguments]\n", progName);
    printf("   commands:\n");
    printf("       --add <hunt_id>: Add a new treasure to the specified hunt (game session).\n\t\t\tEach hunt is stored in a separate directory. \n");
    printf("       --list <hunt_id>: List all treasures in the specified hunt.\n\t\t\t First print the hunt name, the (total) file size and last modification time of its treasure file(s), then list the treasures.\n");
    printf("       --view <hunt_id> <id>: View details of a specific treasure\n");
    printf("       --remove_treasure <hunt_id> <id>: Remove a treasure\n");
    printf("       --remove_hunt <hunt_id>: Remove an entire hunt\n");
    printf("\n");
}

int main(int argc, char *argv[])
{

    if (argc < 2)
    {
        print_help(argv[0]);
        return TREASURE_FAILURE;
    }
    else
    {
        treasureManagerCommand command;

        if (strcmp(argv[1], "--add") == 0)
            command = ADD;
        else if (strcmp(argv[1], "--list") == 0)
            command = LIST;
        else if (strcmp(argv[1], "--view") == 0)
            command = VIEW;
        else if (strcmp(argv[1], "--remove_treasure") == 0)
            command = REMOVE_TREASURE;
        else if (strcmp(argv[1], "--remove_hunt") == 0)
            command = REMOVE_HUNT;
        else
        {
            printf("Invalid command.\n");
            print_help(argv[0]);
            return TREASURE_FAILURE;
        }

        switch (command)
        {
        case ADD:
            if ((argc != 9) || (strcmp(argv[2], "") == 0)  || ((strcmp(argv[3], "") == 0)) || ((strcmp(argv[4], "") == 0)) || ((strcmp(argv[5], "") == 0))  || ((strcmp(argv[6], "") == 0)) || ((strcmp(argv[7], "") == 0)))
            {
                printf("Usage: %s --add <hunt_id:char*> <treasure:char*> <user:char*> <lat:float> <long:float> <clue:char*> <value:int>\n", argv[0]);
                return TREASURE_FAILURE;
            }
            else
            {
                float lat = atof(argv[5]);
                float lon = atof(argv[6]);
                int value = atoi(argv[8]);

                add_treasure(argv[2], argv[3], argv[4], lat, lon, argv[7], value);
            }
            break;
        case LIST:
            if ((argc < 2) || (strcmp(argv[2], "") == 0)) {
                printf("Usage: %s --list <hunt_id:char*>\n", argv[0]);
                return TREASURE_FAILURE;
            } else 
                list_treasures(argv[2]);
            break;
        case VIEW:
            if ((argc < 3) || ((strcmp(argv[2], "") == 0)) || ((strcmp(argv[3], "") == 0))) {
                printf("Usage: %s --view <hunt_id:char*> <treasure:char*>\n", argv[0]);
                return TREASURE_FAILURE;
            } else
                view_treasure(argv[2], argv[3]);
            break;
        case REMOVE_TREASURE:
            if ((argc < 3) || ((strcmp(argv[2], "") == 0)) || ((strcmp(argv[3], "") == 0))) {
                printf("Usage: %s --remove_treasure <hunt_id:char*> <treasure:char*>\n", argv[0]);
                return TREASURE_FAILURE;
            } else
                remove_treasure(argv[2], argv[3]);
            break;
        case REMOVE_HUNT:
            if ((argc < 2) || (strcmp(argv[2], "") == 0)) {
                printf("Usage: %s --remove_hunt <hunt_id:char*> \n", argv[0]);
                return TREASURE_FAILURE;
            } else
                remove_hunt(argv[2]);
            break;
        default:
            printf("Comanda nedefinita!\n");
            print_help(argv[0]);
            break;
        }
    }

    return TREASURE_SUCCESS;
}