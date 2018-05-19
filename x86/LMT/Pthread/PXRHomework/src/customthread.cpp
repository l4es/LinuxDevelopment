#include "customthread.h"
using namespace std;

pthread_mutex_t CustomThread::m_Mutex;

CustomThread::CustomThread() {
}

CustomThread::~CustomThread() {
}


int CustomThread::create(void *callback, void *args) {
  int ret = 0;
 
  //Supercreepy typecast
  ret = pthread_create(&m_ThreadId, NULL, (void *(*)(void *))callback, args);

  if(ret) { 
    cerr << "Error while creating thread." << endl;
    return ret;
  }
  else {
 //   cout << "Thread successfully created." << endl;
    return 0;
  }
}

int CustomThread::join() {
  pthread_join(m_ThreadId, NULL);
  return 0;
}

int CustomThread::detach() {
  pthread_detach(m_ThreadId);
  return 0;
}

int CustomThread::initMutex() {
  
  if(pthread_mutex_init(&CustomThread::m_Mutex, NULL) < 0) {
    cerr << "Error while initializing mutex" << endl;
    return -1;
  }
  else {
//    cout << "Mutex initialized." << endl;
    return 0;
  }
}

int CustomThread::lockMutex(const char *identifier) {
//  cout << identifier << " is trying to acquire the lock..." << endl;
  if(pthread_mutex_lock(&CustomThread::m_Mutex) == 0) {
//    cout << identifier << " acquired the lock!" << endl;
    return 0;
  }
  else {
   cerr << "Error while " << identifier << " was trying to acquire the lock" << endl;
   return -1;
  }
}

int CustomThread::unlockMutex(const char *identifier) {
 // cout << identifier << " is trying to release the lock..." << endl;
  if(pthread_mutex_unlock(&CustomThread::m_Mutex) == 0) {
//    cout << identifier << " released the lock!" << endl;
    return 0;
  }
  else {
   cerr << "Error while " << identifier << " was trying to release the lock" << endl;
   return -1;
  }
}
