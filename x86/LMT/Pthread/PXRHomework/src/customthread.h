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
    CustomThread();
    virtual ~CustomThread();
    int create(void *callback, void *args);
    int join();
    int detach();

	// Some static guard functions, are not really part of this class
    static int initMutex();
    static int lockMutex(const char *identifier);
    static int unlockMutex(const char *identifier);

  private:
    pthread_t m_ThreadId;
    static pthread_mutex_t m_Mutex;
};

#endif
