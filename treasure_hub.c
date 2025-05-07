#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "sys/wait.h"
#include "signal.h"
#include "dirent.h"
#include "time.h"

#define TREASURE_HUB_SUCCESSFUL 0
#define TREASURE_HUB_FAILURE 1

#define MAX_COMMAND_LEN 25
#define MAX_DIR_LEN 100
#define MONITOR_COMMAND_FILE "monitor_command.txt"

#define NEW_MONITOR_COMMAND SIGUSR1
#define MONITOR_COMPLETE_SIGNAL SIGCHLD

// all the commands supported by the treasure hub program
typedef enum
{
    UNDEFINED,
    START_MONITOR,
    LIST_HUNTS,
    LIST_TREASURES,
    VIEW_TREASURES,
    STOP_MONITOR,
    EXIT
} treasure_hub_command;
// commands supported by the monitor process
typedef enum
{
    MONITOR_NO_OPERATION,
    MONITOR_LIST_HUNTS,
    MONITOR_VIEW_HUNTS,
    MONITOR_LIST_TREASURES,
    MONITOR_VIEW_TREASURES
} treasure_hub_monitor_command;

volatile sig_atomic_t recvNewMonitorCommand = 0;
volatile sig_atomic_t monitorDoneWork = 0;
volatile sig_atomic_t monitorProcessAlive = 1;

pid_t monitorProcessPID = -1;
pid_t parentProcessPID = -1;

treasure_hub_monitor_command newMonitorCommand = MONITOR_NO_OPERATION;

treasure_hub_command newTextToCommand(char *text)
{
    if (strcmp(text, "start_monitor") == 0)
        return START_MONITOR;
    else if (strcmp(text, "list_hunts") == 0)
        return LIST_HUNTS;
    else if (strcmp(text, "list_treasures") == 0)
        return LIST_TREASURES;
    else if (strcmp(text, "view_treasures") == 0)
        return VIEW_TREASURES;
    else if (strcmp(text, "stop_monitor") == 0)
        return STOP_MONITOR;
    else if (strcmp(text, "exit") == 0)
        return EXIT;
    else
        return UNDEFINED;
}

void print_help()
{
    printf("\n  commands:\n");
    printf("    start_monitor, list_hunts, list_treasures, view_treasure, stop_monitor, exit\n");
}

void monitorProcessSignalHandler(int sig_no)
{
    if (sig_no == NEW_MONITOR_COMMAND)
    {
        recvNewMonitorCommand = 1;
        monitorDoneWork = 0;
    }
    else if (sig_no == MONITOR_COMPLETE_SIGNAL)
    {
        recvNewMonitorCommand = 0;
        monitorDoneWork = 1;
    }
    else if (sig_no == SIGSTOP)
    {
        monitorProcessAlive = 0;
    }
    else if (sig_no == SIGINT)
    {
        kill(monitorProcessPID, SIGINT);
    }
}

void monitorProcessCode()
{
    while (monitorProcessAlive)
    {
        if (recvNewMonitorCommand)
        {
            char command_buffer[MAX_COMMAND_LEN] = {0};
            FILE *cmdf = fopen(MONITOR_COMMAND_FILE, "r");
            if (cmdf)
            {
                fgets(command_buffer, sizeof(command_buffer), cmdf);
                command_buffer[strcspn(command_buffer, "\n")] = 0;
                fclose(cmdf);
                remove(MONITOR_COMMAND_FILE);
            }
            else
            {
                perror("Monitor: failed to read command file");
                continue;
            }

            if (strcmp(command_buffer, "list_hunts") == 0)
                newMonitorCommand = MONITOR_LIST_HUNTS;
            else if (strcmp(command_buffer, "list_treasures") == 0)
                newMonitorCommand = MONITOR_LIST_TREASURES;
            else if (strcmp(command_buffer, "view_treasures") == 0)
                newMonitorCommand = MONITOR_VIEW_TREASURES;
            else
                newMonitorCommand = MONITOR_NO_OPERATION;

            switch (newMonitorCommand)
            {
            case MONITOR_LIST_HUNTS:
            {
                printf("[DEBUG]: MONITOR_LIST_HUNTS\n");
                struct dirent *de;
                DIR *dr = opendir(".");
                if (dr == NULL)
                {
                    printf("Could not open current directory");
                    return;
                }
                while ((de = readdir(dr)) != NULL)
                {
                    struct stat stbuf;
                    stat(de->d_name, &stbuf);
                    if (S_ISDIR(stbuf.st_mode) &&
                        strcmp(de->d_name, ".") != 0 &&
                        strcmp(de->d_name, "..") != 0 &&
                        strcmp(de->d_name, ".git") != 0 &&
                        S_ISLNK(stbuf.st_mode) == 0)
                    {
                        printf("Launching ./treasure_manager --list %s\n", de->d_name);
                        pid_t pid = fork();
                        if (pid == 0)
                        {
                            execl("./treasure_manager", "treasure_manager", "--list", de->d_name, NULL);
                            perror("execl failed");
                            exit(1);
                        }
                    }
                }
                closedir(dr);
                while (wait(NULL) > 0)
                    ;
                kill(parentProcessPID, MONITOR_COMPLETE_SIGNAL);
            }
            break;
            case MONITOR_LIST_TREASURES:
            {
                printf("[DEBUG]: MONITOR_LIST_TREASURES\n");
                struct dirent *de;
                DIR *dr = opendir(".");
                if (dr == NULL)
                {
                    printf("Could not open current directory");
                    return;
                }
                while ((de = readdir(dr)) != NULL)
                {
                    struct stat stbuf;
                    stat(de->d_name, &stbuf);
                    if (S_ISDIR(stbuf.st_mode) &&
                        strcmp(de->d_name, ".") != 0 &&
                        strcmp(de->d_name, "..") != 0 &&
                        strcmp(de->d_name, ".git") != 0 &&
                        S_ISLNK(stbuf.st_mode) == 0)
                    {
                        printf("Launching ./treasure_manager --list %s\n", de->d_name);
                        pid_t pid = fork();
                        if (pid == 0)
                        {
                            execl("./treasure_manager", "treasure_manager", "--list", de->d_name, NULL);
                            perror("execl failed");
                            exit(1);
                        }
                    }
                }
                closedir(dr);
                while (wait(NULL) > 0);
                kill(parentProcessPID, MONITOR_COMPLETE_SIGNAL);
            }
            case MONITOR_VIEW_TREASURES:
            {
                printf("[DEBUG]: MONITOR_LIST_TREASURES\n");
                struct dirent *de;
                DIR *dr = opendir(".");
                if (dr == NULL)
                {
                    printf("Could not open current directory");
                    return;
                }
                while ((de = readdir(dr)) != NULL)
                {
                    struct stat stbuf;
                    stat(de->d_name, &stbuf);
                    if (S_ISDIR(stbuf.st_mode) &&
                        strcmp(de->d_name, ".") != 0 &&
                        strcmp(de->d_name, "..") != 0 &&
                        strcmp(de->d_name, ".git") != 0 &&
                        S_ISLNK(stbuf.st_mode) == 0)
                    {
                        printf("Launching ./treasure_manager --view %s\n", de->d_name);
                        pid_t pid = fork();
                        if (pid == 0)
                        {
                            execl("./treasure_manager", "treasure_manager", "--view", de->d_name, NULL);
                            perror("execl failed");
                            exit(1);
                        }
                    }
                }
                closedir(dr);
                while (wait(NULL) > 0);
                kill(parentProcessPID, MONITOR_COMPLETE_SIGNAL);
            }
            default:
                break;
            }
        }
    }
}

int main()
{
    print_help();
    char typedText[MAX_COMMAND_LEN];
    treasure_hub_command newCommand = UNDEFINED;

    struct sigaction monitor_command_sig = {0};
    struct sigaction monitor_done_sig = {0};

    monitor_command_sig.sa_handler = monitorProcessSignalHandler;
    monitor_command_sig.sa_flags = SA_RESTART | SA_ONSTACK;
    sigemptyset(&monitor_command_sig.sa_mask);

    monitor_done_sig.sa_handler = monitorProcessSignalHandler;
    monitor_done_sig.sa_flags = SA_RESTART | SA_ONSTACK;
    sigemptyset(&monitor_done_sig.sa_mask);

    sigaction(NEW_MONITOR_COMMAND, &monitor_command_sig, NULL);
    sigaction(MONITOR_COMPLETE_SIGNAL, &monitor_done_sig, NULL);

    parentProcessPID = getpid();

    while (1)
    {
        printf("\n> ");
        scanf("%s", typedText);

        newCommand = newTextToCommand(typedText);

        switch (newCommand)
        {
        case UNDEFINED:
            printf("Not a command\n");
            break;
        case START_MONITOR:
            if (monitorProcessPID == -1)
            {
                newCommand = UNDEFINED;
                monitorProcessPID = fork();
                if (monitorProcessPID == 0)
                {
                    monitorProcessCode();
                    exit(0);
                }
                else
                {
                    printf("The monitor process has been started!\n");
                }
            }
            break;
        case LIST_HUNTS:
            if (monitorProcessPID == -1)
            {
                printf("The monitor process was not started!\n");
            }
            else
            {
                FILE *cmdf = fopen(MONITOR_COMMAND_FILE, "w");
                if (cmdf)
                {
                    fprintf(cmdf, "list_hunts\n");
                }
                fclose(cmdf);
                kill(monitorProcessPID, NEW_MONITOR_COMMAND);
                printf("[DEBUG]: parent Process - LIST_TREASURES \n");
            }
            break;
        case LIST_TREASURES:
            if (monitorProcessPID == -1)
            {
                printf("The monitor process was not started!\n");
            }
            else
            {
                FILE *cmdf = fopen(MONITOR_COMMAND_FILE, "w");
                if (cmdf)
                {
                    fprintf(cmdf, "list_treasures\n");
                }
                fclose(cmdf);
                kill(monitorProcessPID, NEW_MONITOR_COMMAND);
                printf("[DEBUG]: parent Process - LIST_TREASURES \n");
            }
            break;
        case VIEW_TREASURES:
        {
            if (monitorProcessPID == -1)
            {
                printf("The monitor process was not started!\n");
            }
            else
            {
                FILE *cmdf = fopen(MONITOR_COMMAND_FILE, "w");
                if (cmdf)
                {

                    fprintf(cmdf, "view_treasures\n");
                }
                else
                {
                    perror("Failed to write command to file");
                }
                fclose(cmdf);
                kill(monitorProcessPID, NEW_MONITOR_COMMAND);
                printf("[DEBUG]: parent Process - sent command via file and signal.\n");
            }
        }
        break;
        case STOP_MONITOR:
            if (monitorProcessPID == -1)
                printf("The monitor process was not started!\n");
            else
            {
                newCommand = UNDEFINED;
                kill(monitorProcessPID, SIGSTOP);
                monitorProcessPID = -1;
                printf("The monitor process has been stopped!\n");
            }
            break;
        case EXIT:
            if (monitorProcessPID != -1)
                printf("The monitor is still running, please stop it first\n");
            else
            {
                sleep(1);
                exit(0);
            }
            break;
        default:
            break;
        }
    }

    return TREASURE_HUB_SUCCESSFUL;
}