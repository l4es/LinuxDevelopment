#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
// Play around with the buffer size.
// Also see what happens if you comment out the 
//    fflush call below
 
int main(int argc, char * argv[])
{
    FILE *write_fp, *read_fp;
    char *filename;
    char buffer[16];
    int nread;

    read_fp = stdin;
    if (argc > 1)
    {
        filename = argv[1];
        read_fp = fopen(filename, "r");
        if (read_fp == NULL)
        {
            printf("Can't open file %s\n", filename);
            return -1;
        }
    }
    else printf("Type in text to be interpreted as hex\n");
    
  //  write_fp = popen("od -c", "w");
    write_fp = popen("od -h", "w");
    if (write_fp == NULL)
    {
        printf("Can't open pipe\n");
        return -1;
    }
    nread = fread(buffer, 1, sizeof(buffer), read_fp);
    while (nread > 0)
    {
        fwrite(buffer, sizeof(char), nread, write_fp);
        nread = fread(buffer, 1, sizeof(buffer), read_fp);
        printf("Processed one buffer\n");
        fflush(write_fp);
    }
    fclose(read_fp);
    pclose(write_fp);
    exit(0);
}
