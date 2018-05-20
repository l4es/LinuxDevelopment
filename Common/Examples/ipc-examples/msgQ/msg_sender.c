/* The sender program is very similar to msg1.c. In the main set up, delete the
 msg_to_receive declaration and replace it with buffer[BUFSIZ], remove the message
 queue delete and make the following changes to the running loop.
 We now have a call to msgsnd to send the entered text to the queue. */

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
    int running = 1;
    struct my_msg_st some_data;
    int msgid, len;
    char buffer[BUFSIZ];
    int mykey = getuid();
    printf("mykey = %d\n", mykey);


    msgid = msgget((key_t)mykey, 0666 | IPC_CREAT);

    if (msgid == -1) {
        fprintf(stderr, "msgget failed with error: %d\n", errno);
        exit(EXIT_FAILURE);
    }

    
    some_data.my_msg_type = 1;

    while(running) {
        printf("Enter some text or :msg_type ");
        fgets(buffer, BUFSIZ, stdin);
        strcpy(some_data.some_text, buffer);
        if (buffer[0] == ':')
        {
            printf("Change msg_type from: %d\n", 
                some_data.my_msg_type);
            some_data.my_msg_type = atoi(&buffer[1]);
            printf("    to msg_type: %d\n", 
                some_data.my_msg_type);
            continue; 
        }
        len = strlen(some_data.some_text) +1;
        if (msgsnd(msgid, (void *)&some_data, len , 0) == -1) {
            fprintf(stderr, "msgsnd failed\n");
            exit(EXIT_FAILURE);
        }
        if (strncmp(buffer, "end", 3) == 0) {
            running = 0;
        }
    }

    exit(EXIT_SUCCESS);
}
