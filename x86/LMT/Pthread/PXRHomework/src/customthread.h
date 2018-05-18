#ifndef _CUSTOM_THREAD_
#define _CUSTOM_THREAD_

#include <iostream>
#include <string>
#include <pthread.h>
#include <unistd.h>
#include <cstdlib>

using namespace std;

class CustomThread {
  public:
    pthread_t m_ThreadId;

  private:
    static pthread_mutex_t m_Mutex;

  public:
    CustomThread();
    virtual ~CustomThread();
    int create(void *callback, void *args);
    int join();

    static int initMutex();
    static int lockMutex(const char *identifier);
    static int unlockMutex(const char *identifier);
};

#endif