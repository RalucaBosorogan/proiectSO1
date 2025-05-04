#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "signal.h"
#include "dirent.h"

#define TREASURE_HUB_SUCCESSFUL 0
#define TREASURE_HUB_FAILURE 1

#define MAX_COMMAND_LEN 25
#define MAX_DIR_LEN 100

#define NEW_MONITOR_COMMAND SIGUSR1
#define MONITOR_COMPLETE_SIGNAL SIGCHLD

// all the commands supported by the treasure hub program
typedef enum {UNDEFINED, START_MONITOR, LIST_HUNTS, LIST_TREASURES, VIEW_TREASURES, STOP_MONITOR, EXIT} treasure_hub_command;
// commands supported by the monitor process
typedef enum {MONITOR_NO_OPERATION, MONITOR_LIST_HUNTS, MONITOR_VIEW_HUNTS, MONITOR_LIST_TREASURES, MONITOR_VIEW_TREASURES} treasure_hub_monitor_command;

volatile sig_atomic_t recvNewMonitorCommand = 0;
volatile sig_atomic_t monitorDoneWork = 0;

// communication pipe to senf commands from parent to child
int fd[2];

pid_t monitorProcessPID = -1;
pid_t parentProcessPID = -1;

treasure_hub_command newTextToCommand(char *text) {
    if (strcmp(text, "start_monitor") == 0)
    {
        return START_MONITOR;
    }
    else if (strcmp(text, "list_hunts") == 0)
    {
        return LIST_HUNTS;
    }
    else if (strcmp(text, "list_treasures") == 0)
    {
       return LIST_TREASURES;
    }
    else if (strcmp(text, "view_treasures") == 0)
    {
       return VIEW_TREASURES;
    }
    else if (strcmp(text, "stop_monitor") == 0)
    {
        return STOP_MONITOR;
    }
    else if (strcmp(text, "exit") == 0)
    {
        return EXIT;
    }
    else
    {
        return UNDEFINED;
    }
}


void print_help() {
    printf(" commands:\n");
    printf("    start_monitor:\n");
    printf("             starts a separate background process that monitors the hunts and prints to the standard output information about them when asked to\n");
    printf("    list_hunts:\n");
    printf("             asks the monitor to list the hunts and the total number of treasures in each\n");
    printf("    list_treasures:\n");
    printf("             tells the monitor to show the information about all treasures in a hunt, the same way as the command line at the previous stage did\n");
    printf("    view_treasure: \n");
    printf("             tells the monitor to show the information about a treasure in hunt, the same way as the command line at the previous stage did\n");
    printf("    stop_monitor:\n");
    printf("             asks the monitor to end then returns to the prompt. Prints monitor's  termination state when it ends\n");
    printf("    exit:\n");
    printf("             if the monitor still runs, prints an error message, otherwise ends the program\n");
}

void monitorProcessSignalHandler(int sig_no) {
    if (sig_no == NEW_MONITOR_COMMAND) {
        recvNewMonitorCommand = 1;
    }

    if (sig_no == MONITOR_COMPLETE_SIGNAL) {
        monitorDoneWork = 1;
        recvNewMonitorCommand = 0;
    }

}

void monitorProcessCode() {
    while (1)
    {
        if (recvNewMonitorCommand) {
            treasure_hub_monitor_command temp;
            read(fd[0],&temp,sizeof(treasure_hub_monitor_command));
            recvNewMonitorCommand = 0;
            //printf("[Child] Got new command\n");
            switch (temp)
            {
            case MONITOR_LIST_HUNTS:
                execl("./","treasure_manager","--list","Hunt01",NULL);
                kill(parentProcessPID,MONITOR_COMPLETE_SIGNAL);
                break;
            case MONITOR_LIST_TREASURES:
                execl("./","treasure_manager","--list","Hunt01",NULL);
                kill(parentProcessPID,MONITOR_COMPLETE_SIGNAL);
                break;
            case MONITOR_VIEW_HUNTS:
                execl("./","treasure_manager","--view","Hunt01","Treasure1",NULL);
                kill(parentProcessPID,MONITOR_COMPLETE_SIGNAL);
                break;
            case MONITOR_VIEW_TREASURES:
                execl("./","treasure_manager","--view","Hunt01",NULL);
                kill(parentProcessPID,MONITOR_COMPLETE_SIGNAL);
                break;
            default:
                break;
            }

        } else 
            pause();
        
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
    monitor_done_sig.sa_flags = SA_RESTART | SA_ONSTACK | SA_NOCLDSTOP;
    sigemptyset(&monitor_done_sig.sa_mask);

    sigaction(NEW_MONITOR_COMMAND, &monitor_command_sig, NULL);
    sigaction(MONITOR_COMPLETE_SIGNAL, &monitor_done_sig, NULL);

    parentProcessPID = getpid();

    while (1)
    {
        printf("\n> ");
        scanf("%s",typedText);

        newCommand = newTextToCommand(typedText);

        switch(newCommand) {
            case UNDEFINED:
                printf("Not a command\n");
                break;
            case START_MONITOR:
                if (monitorProcessPID == -1) {
                    monitorProcessPID = fork();

                    if (monitorProcessPID == 0) {
                        monitorProcessCode();
                    } else {
                        printf("The monitor process has been started!\n");
                    }
                } 
                break;
            case LIST_HUNTS:
                if (monitorProcessPID == -1)
                    printf("The monitor process was not started!\n");
                else {
                    treasure_hub_monitor_command temp = MONITOR_LIST_HUNTS;
                    write(fd[1],&temp, sizeof(treasure_hub_monitor_command));
                    kill(monitorProcessPID,NEW_MONITOR_COMMAND);
              
                }
                break;
            case LIST_TREASURES:
                if (monitorProcessPID == -1)
                    printf("The monitor process was not started!\n");
                else {
                    treasure_hub_monitor_command temp = MONITOR_LIST_TREASURES;
                    write(fd[1],&temp, sizeof(treasure_hub_monitor_command));
                }
                break;
            case VIEW_TREASURES:
                if (monitorProcessPID == -1)
                    printf("The monitor process was not started!\n");
                else
                {
                    treasure_hub_monitor_command temp = MONITOR_VIEW_TREASURES;
                    write(fd[1], &temp, sizeof(treasure_hub_monitor_command));
                }
                break;
            case STOP_MONITOR:
                if (monitorProcessPID == -1)
                    printf("The monitor process was not started!\n");
                else {
                    kill(monitorProcessPID,SIGSTOP);
                    monitorProcessPID = -1;

                }
                break;
            case EXIT:
                if (monitorProcessPID != -1)
                    printf("The monitor is still running, please stop it first\n");
                else  {
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