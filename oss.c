/*
Jacob Drew
4760 Project 6
oss.c

*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <string.h>

// struct to hold the clock
struct simClock
{
    int clockSeconds;
    int clockNS;
};

// structure for message queue
struct mesg_buffer
{
    long mesg_type;
    char mesg_text[500];
} message;

// struct for the page table entries
struct pageTable
{
    int page[32]; // representing 32k of memory per process
    int sizeDelimiter;
    int dataField;
};

// struct for the frame table entries
struct frameAlloc
{
    int page;
    int referenceByte;
    int dirtyBitFlag;
};

struct frameAlloc frameAllocTable[256]; // array of frame allocation structures

// global variables
pid_t *children;
int slave_max;
int maxChildren;

// globals relating to shared memory
int shmidTime;
struct simClock *shmTime; // shared memory for clock

int shmPcbID;
struct ProcessControlBlock *shmPCB; // shared memory for process control block

int main(int argc, char *argv[])
{

    int i;
    int n;
    maxChildren = 1;
    n = 1;

    // FILE *fp;
    // char *logFileName = "logfile";

    // declare variables
    int opt;

    // getopt
    // process command line args
    while ((opt = getopt(argc, argv, ":t:h")) != -1)
    {
        switch (opt)
        {
        case 'h':
            printf("Help:\n");
            printf("How to run:\n");
            printf("oss [-t n] [-h]\n");
            printf("n number of slave processes to execute\n");
            printf("If n is over 20 it will be set to 18 for safety!\n");
            // if -h is the only arg exit program
            if (argc == 2)
            {
                exit(0);
            }
            break;
        case 't':
            n = atoi(optarg);
            if (n > 18)
            {
                n = 18;
                printf("Number of processes set to 18 for safety\n");
                // printf("N: %d\n", n);
            }
            break;
        case ':':
            printf("option needs a value\n");
            break;
        case '?':
            printf("unknown option: %c\n", optopt);
            break;
        }
    }

    // shared memory keys
    key_t shmkeyTime;
    // key_t shmkeyPCB;
    key_t msgkey;
    int msgid;

    // ftok to generate unique key
    shmkeyTime = ftok("oss.c", 'J');
    // shmkeyPCB = ftok("oss.c", 'A');
    msgkey = ftok("oss.c", 'C');

    // msgget creates a message queue
    // and returns identifier
    msgid = msgget(msgkey, 0666 | IPC_CREAT);
    message.mesg_type = 1;

    // msgsnd to send message
    msgsnd(msgid, &message, sizeof(message), 0);

    // shared memory initialization
    // initialize clock
    shmidTime = shmget(shmkeyTime, sizeof(struct simClock), 0600 | IPC_CREAT); // create shared memory segment
    shmTime = shmat(shmidTime, NULL, 0);                                       // attatch shared memory
    shmTime->clockSeconds = 0;                                                 // set clock variables to zero
    shmTime->clockNS = 0;

    // initializing pids
    if ((children = (pid_t *)(malloc(n * sizeof(pid_t)))) == NULL)
    {
        errno = ENOMEM;
        perror("children malloc");
        exit(1);
    }
    pid_t pid;

    // create an array of page tables for each process
    // initialize the page table
    struct pageTable pageTableArray[n];

    // pageTableArray[i] should refer to the page table for process i
    for (i = 0; i < n; i++)
    {
        // initialize the page table
        pageTableArray[i].dataField = 0;
        pageTableArray[i].sizeDelimiter = 0;
        int j;
        for (j = 0; j < 32; j++)
        {
            pageTableArray[i].page[j] = 0;
        }
    }

    printf("page tables created\n");

    // fork and exec one process
    for (i = 0; i < 1; i++)
    {
        
        pid = fork();
        if (pid == -1)
        {
            // pid == -1 means error occured
            printf("can't fork, error occured\n");
            perror("fork failed");
            exit(EXIT_FAILURE);
        }
        else if (pid == 0)
        {

            // have oss child attatch to clock
            shmkeyTime = ftok("oss.c", 'J'); // arbitrary key

            shmidTime = shmget(shmkeyTime, sizeof(struct simClock), 0600 | IPC_CREAT); // create shared memory segment
            shmTime = shmat(shmidTime, NULL, 0);                                       // attatch shared memory
            shmTime->clockSeconds = shmTime->clockSeconds + 1;

            char *childNum = malloc(6);
            sprintf(childNum, "%d", i);
            children[i] = pid;
            char *args[] = {"./slave", (char *)childNum, "8", (char *)0};

            // send user process msg from message queue
            // msgsnd(msgid, &message, sizeof(message), 0);
            execvp("./slave", args);
            perror("execvp");
        }
    }

    // waiting for all child processes to finish
    for (i = 0; i < n; i++)
    {
        int status;
        waitpid(children[i], &status, 0);
    }

    msgrcv(msgid, &message, sizeof(message), 1, 0);
    FILE *outputFile;
    outputFile = fopen("logFile", "wb");

    fprintf(outputFile, "Data Received is : \n");
    fprintf(outputFile, "%s\n", message.mesg_text);
    fprintf(outputFile, "Time from shared memory clock: %d\n", shmTime->clockSeconds);
    fclose(outputFile);

    printf("OSS finished successfully\n");
    // to destroy the message queue
    msgctl(msgid, IPC_RMID, NULL);
    free(children);

    shmdt(shmTime);                    // detatch clock
    shmctl(shmidTime, IPC_RMID, NULL); // delete shared memory

    // detatch PCB:
    shmdt(shmPCB);                    // detatch clock
    shmctl(shmPcbID, IPC_RMID, NULL); // delete shared memory

    return 0;
}