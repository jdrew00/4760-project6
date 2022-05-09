/*
Jacob Drew
4760 Project 6
userProcess.c
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <sys/sem.h>
#include <sys/msg.h>

// structure for message queue
struct mesg_buffer
{
    long mesg_type;
    char mesg_text[500];
} message;

int main(int argc, char *argv[])
{

    int pid = getpid();
    char *mypid = malloc(6);
    sprintf(mypid, "%d", pid);

    key_t msgkey;
    int msgid;

    // ftok to generate unique key
    msgkey = ftok("oss.c", 'C');

    // msgget creates a message queue
    // and returns identifier
    msgid = msgget(msgkey, 0666 | IPC_CREAT);

    // get the time:
    char cTestString[100];
    time_t temp;
    struct tm *timeptr;
    temp = time(NULL);
    timeptr = localtime(&temp);

    //random number between 1-2
    int randomnumber;
    int pNum = atoi(argv[1]);
    srand(time(NULL) + (pNum * 10));
    randomnumber = (rand() % 2) + 1;

    //if its even have the message be a read, else have it be a write

    //if I had more time I would use a semaphore to impelment IPC to tell if it was a r or w

    if (randomnumber % 2 == 0)
    {
        // msgrcv to receive message
        msgrcv(msgid, &message, sizeof(message), 1, 0);
        strcat(message.mesg_text, "Master: ");
        strcat(message.mesg_text, "Child pid from child process: ");
        strcat(message.mesg_text, mypid);
        strcat(message.mesg_text, " R ");
        strcat(message.mesg_text, "\n");
        // send user process msg from message queue
        msgsnd(msgid, &message, sizeof(message), 0);
    }
    else
    {
        // msgrcv to receive message
        msgrcv(msgid, &message, sizeof(message), 1, 0);
        strcat(message.mesg_text, "Master: ");
        strcat(message.mesg_text, "Child pid from child process: ");
        strcat(message.mesg_text, mypid);
        strcat(message.mesg_text, " W ");
        strcat(message.mesg_text, "\n");
        // send user process msg from message queue
        msgsnd(msgid, &message, sizeof(message), 0);
    }

    return 0;
}