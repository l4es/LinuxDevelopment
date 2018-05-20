#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <unistd.h>

int main(int argc, char* argv[])
{
    int sem1, sem2, sem3, sem4;
    key_t  ipc_key;
    int my_uid = getuid();
    
    ipc_key = ftok(".", 'A');
    if ((sem1 = semget(ipc_key, 3, IPC_CREAT | 0666)) == -1)
    {
        perror("semget: IPC_CREAT | 0666");
    }
    printf("sem1 identifier %d\n", sem1);
    
    
    if ((sem2 = semget(ipc_key, 3, IPC_CREAT | 
                    IPC_EXCL|0666)) == -1)
    {
        perror("semget: IPC_CREAT | IPC_EXCL|0666");
    }
    printf("sem2 identifier %d\n", sem2);
    
    
    if ((sem3 = semget(my_uid, 3, IPC_CREAT | 
                    IPC_EXCL|0600)) == -1)
    {
        perror("semget: IPC_CREAT | IPC_EXCL|0666");
    }
    printf("sem3 identifier %d\n", sem3);
    
    
    if ((sem4 = semget(IPC_PRIVATE, 3, 0600)) == -1)
    {
        perror("semget: IPC_PRIVATE, 0666");
    }
    printf("sem4 identifier %d\n", sem4);



}
