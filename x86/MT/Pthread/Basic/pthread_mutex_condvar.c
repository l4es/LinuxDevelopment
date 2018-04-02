#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

#define TRUE 1
#define FALSE 0

char n[1024];
pthread_mutex_t lock=PTHREAD_MUTEX_INITIALIZER;
int string_read=FALSE;

pthread_cond_t cond;

void * read1()
{
    while (1) {
        while (string_read);
        pthread_mutex_lock(&lock);
        printf("Enter a string: ");
        scanf("%s",n);
        string_read=TRUE;
        pthread_mutex_unlock(&lock);
        pthread_cond_signal(&cond);
    }
}

void * write1()
{
    while (1) {
        pthread_mutex_lock(&lock);
        while(!string_read)
            pthread_cond_wait(&cond,&lock);
        printf("The string entered is %s\n",n);

        string_read=FALSE;
        pthread_mutex_unlock(&lock);
    }
}
int main()
{
        int status;
        pthread_t tr, tw;

        pthread_create(&tr,NULL,read1,NULL);
        pthread_create(&tw,NULL,write1,NULL);

        pthread_join(tr,NULL);
        pthread_join(tw,NULL);
        return 0;
}
