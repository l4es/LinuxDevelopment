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
#define NUM_PHILOS 5
int semid = -1; /* Semaphore IPC ID */
int sem_display = -1; // semaphore for display output

// ********************************************************

// used to guarantee only one dining philosopher doing printf at a time

 void accessDisplay(increment){
       struct sembuf sops;

       sops.sem_num = 0;
       sops.sem_op = increment;
       sops.sem_flg = SEM_UNDO;
       
       if (semop(sem_display, &sops, 1) < 0)
            perror("semop for display");       
           
       return;
   }
 
   // ************************************************
   // print out which forks are on the table

    void update_display(int philo, int eating)
    {
        int i;
        union semun semarg;

        unsigned short forks[NUM_PHILOS];
        printf("update_display: philo(%d)   eating(%d)\n", philo, eating);

       accessDisplay(-1);
       semarg.array = forks;
       if (semctl(semid, 0, GETALL, semarg) < 0)
        {
             perror("semctl(update_display)");
        }

        printf("------------------------------------------------------\n");
        for (i=0; i < NUM_PHILOS; i++)
        {
            if (i < 10)
                printf("%d    ", i);
             else
                printf("%d   ",i);
        }
        printf("\n");
        for (i=0; i < NUM_PHILOS; i++)
        {
            if (forks[i])
                printf("--E  ");
            else
                printf("     ");
        }
        printf("\n");
        accessDisplay(1);
    }

// ********************************************************

// Called to get 2 adjacent forks or to put them down on the table

   void accessForks(int rightFork, int increment){
       struct sembuf sops[2];
              
       int leftFork = rightFork+1;
       if (leftFork >= NUM_PHILOS) leftFork = 0;

       sops[0].sem_num = leftFork;
       sops[0].sem_op = increment;
       sops[0].sem_flg = SEM_UNDO;

       sops[1].sem_num = rightFork;
       sops[1].sem_op = increment;
       sops[1].sem_flg = SEM_UNDO;
       
       if (semop(semid, sops, 2) < 0)
            perror("semop for 2 forks");       
           
       return;
   }
   void getForks(int rightFork){
       accessForks(rightFork, -1);
    }   
   void putForks(int rightFork){
       accessForks(rightFork, 1);
    }
    
// ****************************************************

// Here is the dining philosopher code

  void start_philo( int number) {
       int nSleep;
       srand(number); // seed random number generator
       
        for (; ; ) 
          {
            // Start out Thinking
             nSleep = abs(rand()) %4;
             sleep(nSleep);
               
            // Enter Eating mode
             getForks(number);
             
             update_display(number,1); // we are now eating

             nSleep = abs(rand()) %8;                                  
             sleep(nSleep);             
            
             // we are about to start thinking
             putForks(number);
             update_display(number,0); 

           }// End of for loop for Philosopher
    }
     
// **********************************************

void free_resources()
{
   union semun semarg;
   
   if (semctl(semid, 0, IPC_RMID, semarg) < 0)
    {
        perror("free_resources: semid ");
    }
    if (sem_display >= 0)
        if (semctl(sem_display, 0, IPC_RMID, semarg) < 0)
            {
                perror("free_resources: sem_display");
            }

}
//**********************************************************
 
  int main(int argc, char * argv[]) {
    union semun semarg;
    unsigned short forks[NUM_PHILOS];
    int children[NUM_PHILOS];
    int i, pid, status;
  
    for (i=0; i < NUM_PHILOS; i++)
    {
        forks[i] = 1;
        children[i] = 0;
    } 


 // ------------------------------- 
 // Get semaphore for 5 forks

    semid = semget(IPC_PRIVATE, 5, 0666);
    if (semid < 0)
    {
        perror("semget(semid)");
        exit(-1);
    } 
    printf("created semid %d\n", semid);

    semarg.array = forks;
    if (semctl(semid, 0, SETALL, semarg) < 0)
    {
        perror("semctl(semid)");
        free_resources();
        exit(-1);
    }
    printf("Initialized fork semaphore\n");
    // ------------------------------

// Get semaphore for display critical region

    sem_display = semget(IPC_PRIVATE, 1, 0666);
    if (sem_display < 0)
    {
        free_resources();
        perror("semget(sem_display)");
        exit(-1);
    }
    printf("created sem_display %d\n", sem_display); 

    semarg.val = 1;
    if (semctl(sem_display, 0, SETALL, &semarg) < 0)
    {
        perror("semctl(sem_display)");
        free_resources();
        exit(-1);
    }
    // -------------------------------
 
    // Finally start up our philosophers
   
    for (i = 0; i < NUM_PHILOS; i++)
    {
            pid = fork();
            if (pid < 0)
                {
                    perror("fork err");
                    free_resources();
                    exit(-1);
                }
            if (pid == 0)
            {
                start_philo(i);
                return 0;
            }
            children[i] = pid;
            printf("Creating child: %d\n", pid);
    }
    
    // Let the philosophers go for 20 seconds  
    sleep(20);       
       
    printf("After 20 Seconds, we Terminate\n");
    for (i=0; i < NUM_PHILOS; i++)
          kill(children[i], SIGTERM);
    
    // Go collect the Philosopher Bodies
      
    for ( ;;)
    {
        pid = 0;
       // For each child still around, execute a call to wait
        for (i = 0; i < NUM_PHILOS; i++)
        {
            if (children[i])
            {
                pid = wait(&status);
                break;
            }
        } 
        // if we didn't find any child, then exit this loop
        if (pid == 0) break;
        printf("freeing up child %d\n", pid);
        for (i = 0; i < NUM_PHILOS; i++)
        {
            if (children[i] == pid)
                children[i] = 0;
        }    
    }
    printf("All done waiting for children\n");

    // Find out if we have any dishonest philosophers
    // Make sure we get all of our forks back  
    // SEM_UNDO should make this happen

    semarg.array = forks;
    for (i=0; i < NUM_PHILOS; i++) 
        forks[i] = 0; // make sure GETALL is really filling fork

    if (semctl(semid, 0, GETALL, semarg) < 0)
        {
             perror("semctl(main)");
        }
    for (i=0; i < NUM_PHILOS; i++)
    {
        if (forks[i] != 1)
            printf("Fork %d was not returned %d\n", i, forks[i]);
    }

    free_resources();


    }//End of main
