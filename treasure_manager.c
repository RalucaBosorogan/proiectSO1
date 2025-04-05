#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <errno.h>

#define TREASURE_FILE "treasures.dat"
#define LOG_FILE "logged_hunt"
#define MAX_PATH 512

typedef struct {
    char treasure_id[32];
    char user[64];
    float latitude;
    float longitude;
    char clue[256];
    int value;
} Treasure;

void log_operation(const char *hunt_id, const char *operation) {
    char log_path[MAX_PATH];
    snprintf(log_path, MAX_PATH, "%s/%s", hunt_id, LOG_FILE);

    int fd = open(log_path, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd == -1) {
        perror("open log file");
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
    symlink(log_path, symlink_name); // ignore failure if it exists
}

void add_treasure(const char *hunt_id, const char *tid, const char *user, float lat, float lon, const char *clue, int value) {
    char dir_path[MAX_PATH];
    snprintf(dir_path, MAX_PATH, "%s", hunt_id);
    mkdir(dir_path, 0755);

    char file_path[MAX_PATH];
    snprintf(file_path, MAX_PATH, "%s/%s", hunt_id, TREASURE_FILE);

    int fd = open(file_path, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd == -1) {
        perror("open treasure file");
        exit(EXIT_FAILURE);
    }

    Treasure t;
    strncpy(t.treasure_id, tid, sizeof(t.treasure_id));
    strncpy(t.user, user, sizeof(t.user));
    t.latitude = lat;
    t.longitude = lon;
    strncpy(t.clue, clue, sizeof(t.clue));
    t.value = value;

    write(fd, &t, sizeof(Treasure));
    close(fd);

    log_operation(hunt_id, "Added treasure");
    printf("Treasure added successfully.\n");
}

void list_treasures(const char *hunt_id) {
    char file_path[MAX_PATH];
    snprintf(file_path, MAX_PATH, "%s/%s", hunt_id, TREASURE_FILE);

    struct stat st;
    if (stat(file_path, &st) == -1) {
        perror("stat");
        return;
    }

    printf("Hunt: %s\n", hunt_id);
    printf("Size: %ld bytes\n", st.st_size);
    printf("Last modified: %s", ctime(&st.st_mtime));

    int fd = open(file_path, O_RDONLY);
    if (fd == -1) {
        perror("open");
        return;
    }

    Treasure t;
    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
        printf("- ID: %s, User: %s, Location: (%.2f, %.2f), Value: %d\n",
            t.treasure_id, t.user, t.latitude, t.longitude, t.value);
    }
    close(fd);
    log_operation(hunt_id, "Listed treasures");
}

void view_treasure(const char *hunt_id, const char *id) {
    char file_path[MAX_PATH];
    snprintf(file_path, MAX_PATH, "%s/%s", hunt_id, TREASURE_FILE);

    int fd = open(file_path, O_RDONLY);
    if (fd == -1) {
        perror("open");
        return;
    }

    Treasure t;
    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
        if (strcmp(t.treasure_id, id) == 0) {
            printf("ID: %s\nUser: %s\nLat: %.2f\nLong: %.2f\nClue: %s\nValue: %d\n",
                t.treasure_id, t.user, t.latitude, t.longitude, t.clue, t.value);
            log_operation(hunt_id, "Viewed treasure");
            close(fd);
            return;
        }
    }
    printf("Treasure not found.\n");
    close(fd);
}

void remove_treasure(const char *hunt_id, const char *id) {
    char file_path[MAX_PATH];
    snprintf(file_path, MAX_PATH, "%s/%s", hunt_id, TREASURE_FILE);

    int fd = open(file_path, O_RDONLY);
    if (fd == -1) {
        perror("open");
        return;
    }

    char temp_path[MAX_PATH];
    snprintf(temp_path, MAX_PATH, "%s/temp.dat", hunt_id);
    int tmp = open(temp_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    Treasure t;
    int found = 0;
    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
        if (strcmp(t.treasure_id, id) != 0) {
            write(tmp, &t, sizeof(Treasure));
        } else {
            found = 1;
        }
    }
    close(fd);
    close(tmp);
    rename(temp_path, file_path);

    if (found) {
        printf("Treasure removed.\n");
        log_operation(hunt_id, "Removed treasure");
    } else {
        printf("Treasure not found.\n");
    }
}

void remove_hunt(const char *hunt_id) {
    char file_path[MAX_PATH];
    snprintf(file_path, MAX_PATH, "%s/%s", hunt_id, TREASURE_FILE);
    unlink(file_path);

    char log_path[MAX_PATH];
    snprintf(log_path, MAX_PATH, "%s/%s", hunt_id, LOG_FILE);
    unlink(log_path);

    char symlink_name[MAX_PATH];
    snprintf(symlink_name, MAX_PATH, "logged_hunt-%s", hunt_id);
    unlink(symlink_name);

    rmdir(hunt_id);
    printf("Hunt '%s' removed.\n", hunt_id);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s --<command> <hunt_id> [additional arguments]\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (strcmp(argv[1], "--add") == 0) {
        if (argc != 9) {
            fprintf(stderr, "Usage: %s --add <hunt_id> <treasure_id> <user> <lat> <lon> <clue> <value>\n", argv[0]);
            return EXIT_FAILURE;
        }
        float lat = atof(argv[5]);
        float lon = atof(argv[6]);
        int value = atoi(argv[8]);
        add_treasure(argv[2], argv[3], argv[4], lat, lon, argv[7], value);
    } else if (strcmp(argv[1], "--list") == 0) {
        list_treasures(argv[2]);
    } else if (strcmp(argv[1], "--view") == 0 && argc == 4) {
        view_treasure(argv[2], argv[3]);
    } else if (strcmp(argv[1], "--remove_treasure") == 0 && argc == 4) {
        remove_treasure(argv[2], argv[3]);
    } else if (strcmp(argv[1], "--remove") == 0) {
        remove_hunt(argv[2]);
    } else {
        fprintf(stderr, "Invalid command or arguments.\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
