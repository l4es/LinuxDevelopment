/* workers.c -- Workers/Tools Demo */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/wait.h>

 union semun {
        int val;                    /* value for SETVAL */
        struct semid_ds *buf;       /* buffer for IPC_STAT, IPC_SET */
        unsigned short int *array;  /* array for GETALL, SETALL */
        struct seminfo *__buf;      /* buffer for IPC_INFO */
    };

#define N_SEC   3       /* Run simulation for n seconds */

static int semid = -1;  /* Semaphore IPC ID */

/*
 * Worker List :
 */
static struct {
    char    *name;      /* Worker name */
    pid_t   PID;        /* Process ID */
} workers[] = {
    { "Paul", 0 },
    { "Alfred", 0 },
    { "Robert", 0 },
    { "Adam", 0 }
};

/*
 * Tool names :
 */
static char *tools[] = {
    "Hammer",
    "Screwdriver",
    "Chisel"
};

/*
 * In this case we just want to notify the control
 * semaphore (#3). The control semaphore prevents
 * other processes from reporting something to
 * stdout until the present process is finished its
 * reporting to stdout. This keeps the report serialized.
 */
static void
notifyControl(void) {
    int z;
    static struct sembuf sops = { 3, 1, 0 };

    z = semop(semid,&sops,1);
    if ( z == -1 ) {    
        perror("semop(notify ctl)");
        exit(13);
    }
}

/*
 * This function randomly selects 1 to 3 tools that
 * will be required. When hammers or chisels are
 * selected, only require one tool. For screwdrivers
 * allow one or two screwdrivers to be required.
 */
static struct sembuf *
identifyToolsNeeded(int *n) {
    int x, y, sem;
    static struct sembuf sops[3];

    /*
     * Determine the number of tools required:
     */
    *n = rand() % 3 + 1;    /* 1 to 3 */

    /*
     * Now uniquely define the tools needed:
     */
    for ( x=0; x<*n; ++x ) {
        do  {
            sem = rand() % 3;
            for ( y=0; y<x; ++y )
                if ( sops[y].sem_num == sem ) {
                    /* Already used */
                    sem = -1;
                    break;
                }
    } while ( sem == -1 );
        sops[x].sem_num = sem;  /* Tool required */
        if ( sem == 1 ) /* Allow up to 2 screwdrivers */
            sops[x].sem_op = rand() % 2 + 1;
        else            /* All other tools, only one each */
            sops[x].sem_op = -1;
        sops[x].sem_flg = 0;    /* No flags */
    }

    /*
     * The control semaphore :
     */
    sops[*n].sem_num = 3;   /* Sem #3 controls stdout */
    sops[*n].sem_op = -1;   /* Wait operation */
    sops[*n].sem_flg = 0;   /* No flags */

    return sops;
}

/*
 * This function is used to report our tools & state:
 */
void
reportTools(int n,struct sembuf *sops,char *name,char flg) {
    int x;
    int nt;             /* Number of tools */
    char *oper;         /* Operation */
    char buf[1024];     /* Formatting buffer */

    if ( flg == 'W' )
        oper = "waiting for tools";
    else if ( flg == 'X' )
        oper = "using tools";
    else
        oper = "returning tools";
    
    sprintf(buf,"Worker %s is %s\n",
        name,oper);

    /*
     * Only report the tools for the "using"
     * status report:
     */
    if ( flg == 'X' ) {
        for ( x=0; x<n; ++x ) {
            nt = abs(sops[x].sem_op);
            sprintf(buf+strlen(buf),
                "  %d %s%s\n",
                nt,
                tools[sops[x].sem_num],
                nt == 1 ? "" : "s");
        }
    }

    /*
     * Write all of our text lines atomically to
     * standard output :
     */
    write(1,buf,strlen(buf));
}

/*
 * This function is run by the child process to
 * simulate one worker needing and returning tools:
 */
static void
toolTime(int workerx,time_t t0) {
    char *name = workers[workerx].name;
    pid_t pid = getpid();   /* Get our process ID */
    int x;                  /* Work int */
    time_t tn;              /* Current time */
    int n_tools;            /* Number of tools required */
    struct sembuf *sops = 0;/* Tool list */

    /*
     * Start random number generator :
     */
    srand((unsigned)pid);

    /*
     * Loop for N_SEC seconds :
     */
    for (;;) {
        /*
         * Check for quitting time :
         */
        time(&tn);
        if ( tn - t0 >= N_SEC )
            return;         /* Quitting time! */

        /*
         * Identify the tools that we need:
         */
        sops = identifyToolsNeeded(&n_tools);
        reportTools(n_tools,sops,name,'W');

        /*
         * Wait for tools to become available:
         * Note: n_tools+1 includes ctl semaphore.
         */
        semop(semid,sops,n_tools+1);
        reportTools(n_tools,sops,name,'X');
        notifyControl();    /* Done with stdout */

        /*
         * Pretend to work :
         */
        sleep(1);

        /*
         * Turn wait sem_ops into notifies for
         * the tools only (we don't use the control
         * semaphore here):
         */
        for ( x=0; x<n_tools; ++x) 
            sops[x].sem_op = -sops[x].sem_op;

        /*
         * Return the tools to the toolbox :
         * [Notify operation]
         */
        reportTools(n_tools,sops,name,'R');
        semop(semid,sops,n_tools);
    }
}

/*
 * Main program :
 */
int
main(int argc,char **argv) {
    int z;                  /* Return code */
    int x;                  /* Work index */
    time_t t0;              /* Start time */
    union semun semarg;     /* semctl() arg */
    /* Initial counts of tools in toolbox: */
    static ushort icounts[] = {
        3,  /* Hammers */
        2,  /* Screwdrivers */
        1,  /* Chisel */
        1   /* Control semaphore */
    };
    ushort tcounts[4];      /* Final tool counts */
    int status;             /* Termination status */
    pid_t chPID;            /* Child process ID */
    
    /*
     * Get a private set of semaphores :
     */
    semid = semget(IPC_PRIVATE,4,0600);
    if ( semid == -1 ) {
        perror("semget()");
        exit(1);
    }

    /*
     * Initialize the semaphore counts :
     */
    semarg.array = icounts;
    z = semctl(semid,0,SETALL,semarg);
    if ( z == -1 ) {
        perror("semctl(SETALL)");
        exit(1);
    }

    /*
     * Record our start time :
     */
    time(&t0);

    /*
     * Now create four worker processes :
     */
    for ( x=0; x<4; ++x ) {
        fflush(stdout);
        fflush(stderr);

        if ( !(chPID = fork()) ) {
            /* Child process: */
            toolTime(x,t0);
            return 0;

        } else if ( chPID == (pid_t)(-1) ) {
            fprintf(stderr,"%s: fork(x=%d)\n",
                strerror(errno),x);
            return 13;
        }
        workers[x].PID = chPID;
    }
    
    /*
     * Now wait for all processes to end :
     */
    do  {
        chPID = wait(&status);
        for ( x=0; x<4; ++x )
            if ( workers[x].PID == chPID ) {
                workers[x].PID = 0;
                break;
            }
        /*
         * See if they have all terminated :
         */
        for ( x=0; x<4; ++x )
            if ( workers[x].PID != 0 )
                break;
    } while ( x < 4 );

    printf("All workers have quit.\n");

    /*
     * Obtain all semaphore counts :
     */
    semarg.array = tcounts;
    z = semctl(semid,0,GETALL,semarg);
    if ( z == -1 ) {
        perror("semctl(GETALL)");
        exit(1);
    }

    /*
     * Check our tool counts :
     */
    for ( x=0; x<3; ++x )
        if ( tcounts[x] != icounts[x] )
            printf("Now have %d %ss. Had initially %d.\n",
                tcounts[x], tools[x], icounts[x]);
        else
            printf("All %d %ss are accounted for.\n",
                tcounts[x], tools[x]);

    puts("Simulation Complete.");

    /*
     * Remove the semaphore set:
     */
    z = semctl(semid,0,IPC_RMID,semarg);
    if ( z == -1 ) {
        perror("semctl(IPC_RMID)");
        exit(1);
    }

    return 0;
}
