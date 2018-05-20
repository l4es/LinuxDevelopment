#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define MAX_TEXT 512

struct my_msg_st {
    long int my_msg_type;
    char some_text[MAX_TEXT];
};

int main()
{
    struct msqid_ds info;
    int running = 1;
    struct my_msg_st some_data;
    int msgid, len, repeat, count;
    char buffer[BUFSIZ];
    int mykey = getuid();
    printf("mykey = %d\n", mykey);


    msgid = msgget((key_t)mykey, 0666 | IPC_CREAT);

    if (msgid == -1) {
        fprintf(stderr, "msgget failed with error: %d\n", errno);
        exit(EXIT_FAILURE);
    }

    // The following can only be done as superuser on Linux --- other
    // UNIX machines allow this 
    
    if (msgctl(msgid, IPC_STAT, &info))
            perror("msgctl IPC_STAT error ");
        
    printf("Message Q *Permission* structure information\n");
    printf("Owners's user ID \t%d\n", info.msg_perm.uid);
    printf("Owner's group ID \t%d\n", info.msg_perm.gid);
    printf("Creator's user ID \t%d\n", info.msg_perm.cuid);
    printf("Creator's group ID\t%d\n", info.msg_perm.cgid);
    printf("Access mode in HEX \t%06X\n", info.msg_perm.mode);
    printf("\n\nAdditional Selected Message Queue Structure Information\n");
    printf("Current # of bytes on queue\t %d\n", info.msg_cbytes);
    printf("Current # of messages on queue\t %d\n", info.msg_qnum);
    printf("Maximum # of bytes on queue\t %d\n", info.msg_qbytes);

// This only works if you have superuser priviledges
    info.msg_qbytes = 32000;
    if (msgctl(msgid, IPC_SET, &info))
            perror("msgctl IPC_SET error ");

    
    some_data.my_msg_type = 1;

    while(running) {
        printf("Enter some text or :msg_type ");
        fgets(buffer, BUFSIZ, stdin);
        strcpy(some_data.some_text, buffer);
        repeat = 1;
        if (buffer[0] == '#')
        {
            repeat = atoi(&buffer[1]);
        }
        len = strlen(some_data.some_text) +1;
       
        for (count = 0; count < repeat; count++)
        {
            if (msgsnd(msgid, (void *)&some_data, len , IPC_NOWAIT) == -1) {
                perror("msgsnd failed ");
                break;
            }
        }
        if (strncmp(buffer, "end", 3) == 0) {
            running = 0;
        }
        

        if (msgctl(msgid, IPC_STAT, &info))
            perror("msgctl error ");
            
        printf("Current # of bytes on queue\t %d\n", info.msg_cbytes);
        printf("Current # of messages on queue\t %d\n", info.msg_qnum);
        printf("Maximum # of bytes on queue\t %d\n", info.msg_qbytes);

    }

    exit(EXIT_SUCCESS);
}


