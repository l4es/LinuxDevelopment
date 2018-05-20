#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <signal.h>

union semun {
        int val;                    /* value for SETVAL */
        struct semid_ds *buf;       /* buffer for IPC_STAT, IPC_SET */
        unsigned short int *array;  /* array for GETALL, SETALL */
        struct seminfo *__buf;      /* buffer for IPC_INFO */
    };
int my_semaphore = -1; 
void start_process();


// ********************************************************

 void accessSema(increment){
       struct sembuf sops;

       sops.sem_num = 0;
       sops.sem_op = increment;
       sops.sem_flg = SEM_UNDO;
       
       if (semop(my_semaphore, &sops, 1) < 0)
            perror("my_semaphore");       
           
       return;
   }
 
   
     
// **********************************************

void free_resources()
{
   union semun semarg;
   
   if (my_semaphore >= 0)
        if (semctl(my_semaphore, 0, IPC_RMID, semarg) < 0)
            {
                perror("free_resources: my_semaphore");
            }

}
//**********************************************************
 
  int main(int argc, char * argv[]) {
    union semun semarg;
    int i, pid, status;

    // ------------------------------

// Get semaphore for display critical region

    my_semaphore = semget(IPC_PRIVATE, 1, 0666);
    if (my_semaphore < 0)
    {
        free_resources();
        perror("semget(my_semaphore)");
        exit(-1);
    }
    printf("created my_semaphore %d\n", my_semaphore); 

// Set a single value to zero --- waiting on this will suspend
// a task until this gets incremented

    semarg.val = 0;
    if (semctl(my_semaphore, 0, SETVAL, semarg) < 0)
    {
        perror("semctl_1(my_semaphore)");
        free_resources();
        exit(-1);
    }
    // -------------------------------
 
    pid = fork();
    if (pid < 0)
     {
        perror("fork err");
        free_resources();
        exit(-1);
      }
     if (pid == 0)
      {
        start_process();
        return 0;
      }
      printf("Creating child: %d\n", pid);
        
    // Let the process go for 5 seconds  
    sleep(5);  
    printf("parent done waiting, now set child in action\n");  
     
    accessSema(6); // Note I don't have to just add 1
    // Go collect the child body
      
    pid = wait(&status);
    printf("All done waiting for child\n");

    
    free_resources();


 }//End of main
 
 void start_process()
 {
    printf("Child calling accessSema(-6)\n");
    accessSema(-6); // I can wait for more than 1
    printf("Child returns from accessSema\n");
    sleep(4);
    printf("Child terminating\n");
 }
