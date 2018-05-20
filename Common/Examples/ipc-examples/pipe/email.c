#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pwd.h>
#include <sys/types.h>


main(int argc,char **argv) {
    FILE *fp;   
    FILE *p = 0;                /* mail pipe */
    char cmd[1000];
    char buffer[1000];
    char * to_addr = "chasselb@stu.wccnet.org";
    char * subject = "from pipe mail";
    char * filename = "names.txt";
    int nread;

    printf("Useage: %s to subject file\n", argv[0]);
    if (argc > 1) to_addr = argv[1];
    if (argc > 2) subject = argv[2];
    if (argc > 3) filename = argv[3];
    printf("Sending mail: to: %s\n subject: %s\n filename %s\n",
        to_addr, subject, filename);
  
    sprintf(cmd,
        "mail -s '%s' %s", subject, to_addr);
    printf("Command to send: %s\n", cmd);

    fp = fopen(filename, "r");
    if (fp == NULL)
    {
        printf("can't open filename %s\n", filename);
        return -1;
    }

    /* Open a pipe to mail: */
    if ( !(p = popen(cmd,"w")) ) {
        fprintf(stderr,  "%s: popen(%s) for write.\n",
            strerror(errno), cmd);
        return -1;
    }
     
    nread = fread(buffer,1, sizeof(buffer), fp);
    while (nread > 0)
    {   
        fwrite(buffer, 1, nread,p);
        nread = fread(buffer, 1, sizeof(buffer), fp);
    }
    fprintf(p,"."); // Ending email conversation
    fclose(fp);
    if ( pclose(p) == -1 ) {
        fprintf(stderr,"%s: pclose(%s)\n", strerror(errno),cmd);
        return -1;
    } else
        printf("Message sent to %s\n",to_addr);

    return 0;
}
