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
#include "ctype.h"

#define TREASURE_HUB_SUCCESSFUL 0
#define TREASURE_HUB_FAILURE 1

#define MONITOR_PROCESS_SUCCESSFUL 0
#define MONITOR_PROCESS_FAILURE 1

#define MAX_COMMAND_LEN 25
#define MAX_DIR_LEN 100
#define MAX_COMMAND_BUFFER_SIZE 2048
#define MAX_TREASURES_PER_HUNT 10
#define MAX_TREASURE_NAME 20
#define MAX_HUNTS 5
#define MAX_HUNT_NAME 10
#define MONITOR_COMMAND_FILE "monitor_command.txt"
#define VIEW_TREASURE_DATA "view_treasure_data.txt"

#define NEW_MONITOR_COMMAND SIGUSR1
#define MONITOR_COMPLETE_SIGNAL SIGCHLD

// delete me and all occurences in the program, this is just 4 debug purposes
int printDebugMessages = 0;

// all the commands supported by the treasure hub program
typedef enum
{
    UNDEFINED,
    START_MONITOR,
    LIST_HUNTS,
    LIST_TREASURES,
    VIEW_TREASURE,
    CALCULATE_SCORE,
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
    MONITOR_VIEW_TREASURE
} treasure_hub_monitor_command;

volatile sig_atomic_t recvNewMonitorCommand = 0;
volatile sig_atomic_t monitorDoneWork = 0;
volatile sig_atomic_t monitorProcessAlive = 1;

pid_t monitorProcessPID = -1;
pid_t parentProcessPID = -1;

char availableHuntsNames[MAX_HUNTS][MAX_HUNT_NAME];
int noHunts = 0;

treasure_hub_command newTextToCommand(char *text)
{
    if (strcmp(text, "start_monitor") == 0)
        return START_MONITOR;
    else if (strcmp(text, "list_hunts") == 0)
        return LIST_HUNTS;
    else if (strcmp(text, "list_treasures") == 0)
        return LIST_TREASURES;
    else if (strcmp(text, "view_treasure") == 0)
        return VIEW_TREASURE;
    else if (strcmp(text, "calculate_score") == 0)
        return CALCULATE_SCORE;
    else if (strcmp(text, "stop_monitor") == 0)
        return STOP_MONITOR;
    else if (strcmp(text, "exit") == 0)
        return EXIT;
    else
        return UNDEFINED;
}

treasure_hub_monitor_command newMonitorTextToCommand(char *text)
{
    if (strcmp(text, "list_hunts") == 0)
        return MONITOR_LIST_HUNTS;
    else if (strcmp(text, "view_hunts") == 0)
        return MONITOR_VIEW_HUNTS;
    else if (strcmp(text, "view_treasure") == 0)
        return MONITOR_VIEW_TREASURE;
    else if (strcmp(text, "list_treasures") == 0)
        return MONITOR_LIST_TREASURES;
    else
        return MONITOR_NO_OPERATION;
}

void print_help()
{
    printf("\n  commands:\n");
    printf("    start_monitor, list_hunts, list_treasures, view_treasure, calculate_score, stop_monitor, exit\n");
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
    else if (sig_no == CLD_EXITED)
    {
        monitorProcessPID = -1;
    }
}

void waitForMonitorToComplete()
{
    do
    {
        usleep(10);
    } while (monitorDoneWork == 0);
}

int getNrOfHunts(char huntNamesArray[MAX_HUNTS][MAX_HUNT_NAME])
{
    int noOfHunts = 0;
    struct dirent *de;

    DIR *dr = opendir(".");

    if (dr == NULL)
    {
        printf("monitorProcessCode() - MONITOR_LIST_HUNTS: Could not open current directory");
    }

    while ((de = readdir(dr)) != NULL)
    {
        struct stat stbuf;
        int ret = 0;

        stat(de->d_name, &stbuf);

        if ((S_ISDIR(stbuf.st_mode) == 1) &&
            (strcmp(de->d_name, ".") != 0) &&
            (strcmp(de->d_name, "..") != 0) &&
            (strcmp(de->d_name, ".git") != 0) &&
            (S_ISLNK(stbuf.st_mode) == 0))
        {
            if (huntNamesArray != NULL)
                strcpy(huntNamesArray[noOfHunts++], de->d_name);
            else
                noOfHunts++;
        }
    }
    return noOfHunts;
}

int getNoOfTreasures(char *commandOutput, char namesOfTreasures[MAX_TREASURES_PER_HUNT][MAX_TREASURE_NAME])
{
    char *temp = commandOutput;
    char *treasureNameBegin = NULL;
    int no_of_treasures = 0;
    int treasureNameLen = 0;
    while ((temp = strstr(temp, "- ID: ")))
    {
        if (namesOfTreasures != NULL)
        {
            treasureNameBegin = temp + 6;

            while (*treasureNameBegin != ',')
            {
                namesOfTreasures[no_of_treasures][treasureNameLen++] = *treasureNameBegin;
                treasureNameBegin++;
            }

            namesOfTreasures[no_of_treasures][treasureNameLen++] = '\0';
            treasureNameLen = 0;
        }
        temp++;
        no_of_treasures++;
    }

    return no_of_treasures;
}

void monitorProcessCode()
{
    treasure_hub_monitor_command newMonitorCommand = MONITOR_NO_OPERATION;

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
                newMonitorCommand = newMonitorTextToCommand(command_buffer);
            }
            else
                newMonitorCommand = MONITOR_NO_OPERATION;

            switch (newMonitorCommand)
            {
            case MONITOR_LIST_HUNTS:
                if (printDebugMessages)
                    printf("[DEBUG]: MONITOR_LIST_HUNTS\n");

                for (int i = 0; i < noHunts; i++)
                {
                    printf("\nLaunching ./treasure_manager --list %s\n", availableHuntsNames[i]);
                    int ret_execl = 0;
                    // command output pipe
                    int commandOutputPipe[2];
                    char commandOutputBuffer[MAX_COMMAND_BUFFER_SIZE] = "";

                    pipe(commandOutputPipe);

                    pid_t pid = fork();

                    if (pid == 0)
                    {
                        close(commandOutputPipe[0]);
                        dup2(commandOutputPipe[1], STDOUT_FILENO);

                        ret_execl = execl("./treasure_manager", "treasure_manager", "--list", availableHuntsNames[i], NULL);

                        if ((printDebugMessages) && (ret_execl != 0))
                            perror("./treasure_manager --list FAILED");
                    }
                    else
                    {
                        close(commandOutputPipe[1]);
                        read(commandOutputPipe[0], commandOutputBuffer, MAX_COMMAND_BUFFER_SIZE);
                        close(commandOutputPipe[0]);

                        printf("The hunt \"%s\" has %d treasures\n", availableHuntsNames[i], getNoOfTreasures(commandOutputBuffer, NULL));
                        printf("%s\n", commandOutputBuffer);
                    }
                }
                wait(NULL);
                if (printDebugMessages)
                    printf("[DEBUG] monitor process send MONITOR_COMPLETE_SIGNAL\n");
                kill(parentProcessPID, MONITOR_COMPLETE_SIGNAL);
                break;
            case MONITOR_LIST_TREASURES:
                if (printDebugMessages)
                    printf("[DEBUG]: MONITOR_LIST_HUNTS\n");

                for (int i = 0; i < noHunts; i++)
                {
                    int ret_execl = 0;
                    // command output pipe
                    int commandOutputPipe[2];
                    char commandOutputBuffer[MAX_COMMAND_BUFFER_SIZE] = "";

                    pipe(commandOutputPipe);

                    pid_t pid = fork();

                    if (pid == 0)
                    {
                        close(commandOutputPipe[0]);
                        dup2(commandOutputPipe[1], STDOUT_FILENO);

                        ret_execl = execl("./treasure_manager", "treasure_manager", "--list", availableHuntsNames[i], NULL);

                        if ((printDebugMessages) && (ret_execl != 0))
                            perror("./treasure_manager --list FAILED");
                    }
                    else
                    {
                        close(commandOutputPipe[1]);
                        read(commandOutputPipe[0], commandOutputBuffer, MAX_COMMAND_BUFFER_SIZE);
                        close(commandOutputPipe[0]);

                        char nameOfTreasure[MAX_TREASURES_PER_HUNT][MAX_TREASURE_NAME];
                        int noOfTreasures = getNoOfTreasures(commandOutputBuffer, nameOfTreasure);

                        printf("The hunt \"%s\" has %d treasures\n", availableHuntsNames[i], noOfTreasures);

                        for (int j = 0; j < noOfTreasures; j++)
                        {
                                pid_t pid = fork();
                                int ret_execl = 0;

                                if (pid == 0)
                                {
                                    ret_execl = execl("./treasure_manager", "treasure_manager", "--view", availableHuntsNames[i], nameOfTreasure[j], NULL);
                                    printf("\n");
                                } else 
                                    wait(NULL);     
                        }
                    }
                }
                wait(NULL);
                if (printDebugMessages)
                    printf("[DEBUG] monitor process send MONITOR_COMPLETE_SIGNAL\n");
                kill(parentProcessPID, MONITOR_COMPLETE_SIGNAL);
                break;
            case MONITOR_VIEW_TREASURE:
                if (printDebugMessages)
                    printf("[DEBUG]: MONITOR_VIEW_TREASURE\n");
                printf("\n");
                
               
  
                if (printDebugMessages)
                    printf("[DEBUG] monitor process send MONITOR_COMPLETE_SIGNAL\n");
                kill(parentProcessPID, MONITOR_COMPLETE_SIGNAL);
                break;
            default:
                break;
            }
        }
    }
    exit(MONITOR_PROCESS_SUCCESSFUL);
}

int main()
{
    // check if treasure manager exists
    struct stat buffer;
    int stat_ret = 0;

    stat_ret = stat("treasure_manager", &buffer);

    if (stat_ret != 0)
    {
        perror("treasure_manager not found");
        printf("\n\tThe treasure_manager program does not exist!!!\n\tPlease make sure the treasure manager program exist in the current folder\n\t\texiting...\n");
        exit(TREASURE_HUB_FAILURE);
    }
    else
    {

        // get all folder(hunt) names
        noHunts = getNrOfHunts(availableHuntsNames);

        if (noHunts != 0)
        {

            // display the list of interactive commands
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
                    printf("Not a command\nPlease type one of the following commands:\n");
                    print_help();
                    break;
                case START_MONITOR:
                    if (monitorProcessPID == -1)
                    {
                        newCommand = UNDEFINED;
                        monitorProcessPID = fork();
                        if (monitorProcessPID == 0)
                        {
                            monitorProcessCode();
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
                        printf("The monitor process was not started!\n\tPlease run the start_monitor command to start it\n");
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
                        monitorDoneWork = 0;
                        if (printDebugMessages)
                            printf("[DEBUG]: parent Process - LIST_HUNTS \n");
                        waitForMonitorToComplete();
                        if (printDebugMessages)
                            printf("[DEBUG]: parent Process - LIST_HUNTS -- DONE \n");
                        continue;
                    }
                    break;
                case LIST_TREASURES:
                    if (monitorProcessPID == -1)
                    {
                        printf("The monitor process was not started!\n\tPlease run the start_monitor command to start it\n");
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
                        monitorDoneWork = 0;
                        if (printDebugMessages)
                            printf("[DEBUG]: parent Process - LIST_TREASURES \n");
                        waitForMonitorToComplete();
                        if (printDebugMessages)
                            printf("[DEBUG]: parent Process - LIST_TREASURES -- DONE \n");
                        continue;
                    }
                    break;
                case VIEW_TREASURE:
                {
                    if (monitorProcessPID == -1)
                    {
                        printf("The monitor process was not started!\n\tPlease run the start_monitor command to start it\n");
                    }
                    else
                    {
                        char givenHuntName[MAX_HUNT_NAME], givenTreasureName[MAX_TREASURE_NAME];
                        
                        printf("\tInsert the name of the hunt(choose from - ");
                        for (int i =0; i < noHunts; i++) 
                            printf("%s ", availableHuntsNames[i]);
                        printf(")\n");

                        printf(">");

                        scanf("%s", givenHuntName);


                       int ret_execl = 0;
                    // command output pipe
                    int commandOutputPipe[2];
                    char commandOutputBuffer[MAX_COMMAND_BUFFER_SIZE] = "";
                    char treasureNames[MAX_TREASURES_PER_HUNT][MAX_TREASURE_NAME];
                    int noOfTreasures = 0;
                    pipe(commandOutputPipe);

                    pid_t pid = fork();
                        if (pid == 0)
                        {
                            close(commandOutputPipe[0]);
                            dup2(commandOutputPipe[1], STDOUT_FILENO);

                            ret_execl = execl("./treasure_manager", "treasure_manager", "--list", givenHuntName, NULL);

                            if ((printDebugMessages) && (ret_execl != 0))
                                perror("./treasure_manager --list FAILED");
                        }
                        else
                        {
                            close(commandOutputPipe[1]);
                            read(commandOutputPipe[0], commandOutputBuffer, MAX_COMMAND_BUFFER_SIZE);
                            close(commandOutputPipe[0]);

                            noOfTreasures = getNoOfTreasures(commandOutputBuffer, treasureNames);
                          
                        }
                        printf("Insert the name of treasure (choose from:");

                        for (int i = 0; i < noOfTreasures;i++)
                            printf("%s ",treasureNames[i]);

                        printf(")\n");

                        scanf("%s",givenTreasureName);

                        FILE *cmdf = fopen(MONITOR_COMMAND_FILE, "w");
                        FILE *view_det = fopen(VIEW_TREASURE_DATA, "w");
                        if (cmdf)
                        {
                            fprintf(cmdf, "view_treasure\n");
                            fprintf(view_det, "%s", givenHuntName);
                            fprintf(view_det,"\n");
                            fprintf(view_det, "%s", givenTreasureName);
                        }
                        else
                        {
                            perror("Failed to write command to file");
                            if (monitorProcessPID != -1)
                            {
                                kill(monitorProcessPID, SIGINT);
                                exit(TREASURE_HUB_FAILURE);
                            }
                        }

                        
                        fclose(cmdf);
                        kill(monitorProcessPID, NEW_MONITOR_COMMAND);
                        usleep(100);
                        monitorDoneWork = 0;
                        if (printDebugMessages)
                            printf("[DEBUG]: parent Process - VIEW_TREASURE \n");
                        waitForMonitorToComplete();
                        if (printDebugMessages)
                            printf("[DEBUG]: parent Process - VIEW_TREASURE -- DONE \n");
                        continue;
                    }
                }
                break;
                case CALCULATE_SCORE:
                    printf("Calculate the score for each hunt\n");
                    struct stat st;
                    int stat_ret = 0,execl_ret = 0;

                    stat_ret = stat("score_calculator", &st);

                    if (stat_ret != 0)
                    {
                        perror("score_calculator not found");
                        printf("\n\tThe score_calculator program does not exist!!!\n\tPlease make sure the score_calculator program exist in the current folder\n\t\texiting...\n");
                        exit(TREASURE_HUB_FAILURE);
                    }

                    for (int i = 0; i < noHunts; i++)
                    {
                        pid_t pid = fork();

                        if (pid == 0) {
                           execl_ret = execl("./score_calculator","score_calculator",availableHuntsNames[i],NULL);
                        } else 
                            wait(NULL);
                    }
                    break;
                case STOP_MONITOR:
                    if (monitorProcessPID == -1)
                        printf("The monitor process was not started!\n\tPlease run the start_monitor command to start it\n");
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
                        printf("The monitor process was not started!\n\tPlease run the start_monitor command to start it\n");
                    else
                    {
                        usleep(500);
                        exit(0);
                    }
                    break;
                default:
                    break;
                }
            }
        }
        else
        {
            printf("No available hunts\n\texiting...");
            return TREASURE_HUB_FAILURE;
        }
    }

    return TREASURE_HUB_SUCCESSFUL;
}