#include <iostream>
#include <sstream>
#include <string>
#include <set>

#include <time.h>
#include <cstring>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>

#include <pthread.h>

#define PROTECT_SET_INSERTION 1

using namespace std;

enum ResultCode
{
    RESULT_OK      = 0,
    RESULT_ERROR   = 1
};

// Forward declaration
class Test;

class InterfaceData { 
private:
	mutable ResultCode resultCode;

public:
	InterfaceData() {}
	InterfaceData( const InterfaceData& interfaceData ) :
	resultCode(RESULT_OK) {}
	
	virtual ~InterfaceData() {}

	InterfaceData operator+(const InterfaceData &rhs) {
		InterfaceData newInterfaceData;
		
		return newInterfaceData;
	}
};

class ThreadData { 
public:
    Test* handlerInstance;
    std::string operationName;
    InterfaceData *opParamData;
};

class ProgressThreadData
{
public:
//    OperationInvoker* invokerInstance;
    std::string operationName;
    pthread_mutex_t* progressMutex;
    pthread_cond_t* progressCond;
    pthread_barrier_t* progressBarrier;
};


class Test { 
public:
    Test() {}
    	
	// handleOperationAsync
	void handleOperation(std::string operationName, InterfaceData paramData) {
		cout << ">> handleOperation " << operationName.c_str() << endl;
		
		InterfaceData *opParamData = new InterfaceData(paramData);

		ThreadData *threadData = new ThreadData;
		threadData->handlerInstance = this;
		threadData->operationName = operationName;
		threadData->opParamData = opParamData;
  
		pthread_t threadId = 0;
		int ret = pthread_create(&threadId,                             //threadId
								NULL,                                  //thread creation attributes
								triggerThreadFunction, 		        //thread function
								static_cast<void*>(threadData));      //args passed to thread function        
		if (ret == 0) {
#ifdef PROTECT_SET_INSERTION
			pthread_mutex_lock(&pthreadsSetMutex);
			cout << "handleOperation::pthreadsSetMutex locked "<< endl;
#endif
				cout << "handleOperation::Insert thread " << threadId << endl;		
				pthreadsSet.insert(threadId);
#ifdef PROTECT_SET_INSERTION
			pthread_mutex_unlock(&pthreadsSetMutex);
			cout << "handleOperation::pthreadsSetMutex unlocked "<< endl;
#endif	
			pthread_detach(threadId);
		}
		else {
			cout << "handleOperation::pthread_create() = " << ret << endl;
			
			delete opParamData;
			delete threadData;
		}
		
		cout << "handleOperation >>"<< endl;
	}

	static void *triggerThreadFunction(void* data) {
		
		ThreadData *threadData = static_cast<ThreadData*>(data);

		InterfaceData opParamData = *(threadData->opParamData);
		std::string operationName = threadData->operationName;

		cout << ">> triggerThreadFunction " << operationName << endl;

		Test* handlerInstance = threadData->handlerInstance;

		delete threadData->opParamData;
		delete threadData;
	
		if (handlerInstance == NULL)
		{
			cout << "handlerInstance == NULL"<< endl;
			return NULL;
		}

		// 1. Start the progress thread
		pthread_t threadId = 0;
		pthread_mutex_t progressMutex = PTHREAD_MUTEX_INITIALIZER;
		pthread_cond_t progressCond = PTHREAD_COND_INITIALIZER;
		pthread_barrier_t progressBarrier;

		pthread_barrier_init(&progressBarrier, NULL, 2);

		ProgressThreadData *progressThreadData = new ProgressThreadData;
//		progressThreadData->invokerInstance = NULL;
		progressThreadData->operationName = operationName;
		progressThreadData->progressMutex = &progressMutex;
		progressThreadData->progressCond = &progressCond;
		progressThreadData->progressBarrier = &progressBarrier;	

		int ret = pthread_create(&threadId,                             	//threadId
								NULL,                                  	//thread creation attributes
								triggerProgressThreadFunction,				//thread function
								static_cast<void*>(progressThreadData)); 	//args passed to thread function        
		if (ret != 0)
		{
			cout << "triggerThreadFunction::pthread_create() = " << ret << endl;
			pthread_barrier_destroy(&progressBarrier);

			delete progressThreadData;
			return NULL;
		}

#ifdef PROTECT_SET_INSERTION
		pthread_mutex_lock(&progressThreadsSetMutex);
		cout << "triggerProgressThreadFunction::progressThreadsSetMutex locked "<< endl;
#endif
			cout << "triggerThreadFunction::Insert thread " << threadId << endl;		
			Test::progressThreadsSet.insert(threadId);
#ifdef PROTECT_SET_INSERTION
		pthread_mutex_unlock(&progressThreadsSetMutex);
		cout << "triggerProgressThreadFunction::progressThreadsSetMutex unlocked "<< endl;
#endif
		cout << "triggerThreadFunction::Wait barrier " << endl;		
		pthread_barrier_wait(&progressBarrier);

		// 3. Start and communicate with the backend script
//		scriptResult = interfaceHandlerInstance->executeBackendScript(operationName, opParamData);

		// 4. Signal and Wait for the progress thread to terminate
		cout << "triggerThreadFunction::Wait mutex " << endl;		
		ret = pthread_mutex_lock(&progressMutex);
		if (ret != 0)
		{
			cout << "triggerThreadFunction::pthread_mutex_lock() = " << ret << endl;

			return NULL;
		}

		cout << "triggerThreadFunction::Signal cond " << endl;		
		ret = pthread_cond_signal(&progressCond);
		if (ret != 0)
		{
			cout << "triggerThreadFunction::pthread_cond_signal() = " << ret << endl;
		}

		cout << "triggerThreadFunction::Release mutex " << endl;		
		ret = pthread_mutex_unlock(&progressMutex);
		if (ret != 0)
		{
			cout << "pthread_mutex_unlock() = " << ret << endl;
		}

		pthread_join(threadId, NULL);
		cout << "triggerThreadFunction::Thread joined : " << threadId << endl;		
		

		cout << "triggerThreadFunction::Destroy barrier " << endl;		
		pthread_barrier_destroy(&progressBarrier);

		// 5. Finalize the Admin operation and report result to BRFC
		pthread_mutex_lock(&pthreadsSetMutex);
			cout << "triggerThreadFunction::pthreadsSetMutex locked "<< endl;

			if(Test::pthreadsSet.find(pthread_self())
			   != Test::pthreadsSet.end())
			{
				cout << "triggerThreadFunction::Erase thread "<< pthread_self() << endl;				
				Test::pthreadsSet.erase(pthread_self());
			}

		pthread_mutex_unlock(&pthreadsSetMutex);
			cout << "triggerThreadFunction::pthreadsSetMutex unlocked "<< endl;
		
		cout << "triggerThreadFunction >>" << endl;

		return NULL;
	}
		
	static void *triggerProgressThreadFunction(void *data) {		
		ProgressThreadData *progressThreadData =
			static_cast<ProgressThreadData*>(data);

//		OperationInvoker* invokerInstance = progressThreadData->invokerInstance;
		std::string operationName = progressThreadData->operationName;
		pthread_mutex_t* progressMutex = progressThreadData->progressMutex;
		pthread_cond_t* progressCond = progressThreadData->progressCond;
		pthread_barrier_t* progressBarrier = progressThreadData->progressBarrier;

		cout << ">> triggerProgressThreadFunction : " << operationName << endl;

		delete progressThreadData;

//		InterfaceData opProgressData;
//		opProgressData.setProgressCountAttributeValue(progressCount);
//		opProgressData.setRequestIdAttributeValue(requestId);

		cout << "triggerProgressThreadFunction::Wait barrier " << endl;		
		pthread_barrier_wait(progressBarrier);
		int rc;
		timespec absTime;

		cout << "triggerProgressThreadFunction::Lock mutex " << endl;		
		rc = pthread_mutex_lock(progressMutex);
		if (rc != 0)
		{
			cout << "triggerThreadFunction::pthread_mutex_lock() = " << rc << endl;
		   return NULL;
		}

		clock_gettime(CLOCK_REALTIME, &absTime);
		absTime.tv_sec = absTime.tv_sec + (20000000/1000000);

		cout << "triggerProgressThreadFunction::Timedwait cond " << endl;		
		while((rc = pthread_cond_timedwait(progressCond, progressMutex, &absTime)) == ETIMEDOUT)
		{
			clock_gettime(CLOCK_REALTIME, &absTime);
			absTime.tv_sec = absTime.tv_sec + (20000000/1000000);
		}

		cout << "triggerProgressThreadFunction::Unlock mutex " << endl;		
		rc = pthread_mutex_unlock(progressMutex);
		if (rc != 0)
		{
			cout << "triggerThreadFunction::pthread_mutex_unlock() = " << rc << endl;
		}

		pthread_mutex_lock(&progressThreadsSetMutex);
		cout << "triggerProgressThreadFunction::progressThreadsSetMutex locked "<< endl;

			if(Test::progressThreadsSet.find(pthread_self()) !=
			   Test::progressThreadsSet.end())
			{
				cout << "triggerProgressThreadFunction::Erase progress thread "<< pthread_self() << endl;				
				Test::progressThreadsSet.erase(pthread_self());
			}

		pthread_mutex_unlock(&progressThreadsSetMutex);
		cout << "triggerProgressThreadFunction::progressThreadsSetMutex unlocked "<< endl;
		
		cout << "triggerProgressThreadFunction >>" << endl;

		return NULL;		
	}
	
	ResultCode finalize()
	{
		pthread_mutex_lock(&pthreadsSetMutex);
		cout << "finalize::pthreadsSetMutex locked "<< endl;

			std::set<pthread_t>::iterator iter;
			for(iter = Test::pthreadsSet.begin();
					iter != Test::pthreadsSet.end(); ++iter)
			{
				cout << "finalize::Cancel thread "<< *iter << endl;				
				pthread_cancel(*iter);
			}
			Test::pthreadsSet.clear();
		pthread_mutex_unlock(&pthreadsSetMutex);
		cout << "finalize::pthreadsSetMutex unlocked "<< endl;

		pthread_mutex_lock(&progressThreadsSetMutex);
		cout << "finalize::progressThreadsSetMutex locked "<< endl;
		
			std::set<pthread_t>::iterator iter2;
			for(iter2 = Test::progressThreadsSet.begin();
					iter2 != Test::progressThreadsSet.end(); ++iter2)
			{
				cout << "finalize::Cancel progress thread "<< *iter2 << endl;				
				pthread_cancel(*iter2);
			}
			Test::progressThreadsSet.clear();
		pthread_mutex_unlock(&progressThreadsSetMutex);
		cout << "finalize::progressThreadsSetMutex locked "<< endl;

		return RESULT_OK;
	}
	

private:
    static std::set<pthread_t> pthreadsSet;
    static pthread_mutex_t pthreadsSetMutex;
    static std::set<pthread_t> progressThreadsSet;
    static pthread_mutex_t progressThreadsSetMutex;
};

std::set<pthread_t> Test::pthreadsSet;
pthread_mutex_t Test::pthreadsSetMutex = PTHREAD_MUTEX_INITIALIZER;
std::set<pthread_t> Test::progressThreadsSet;
pthread_mutex_t Test::progressThreadsSetMutex = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char* argv[]){
	
	Test t;
	InterfaceData testData;
	
	int count= 100;
	if (argc >= 2) {
		count = atoi(argv[1]);
	}

	cout << "Main started " << endl;

	for (int i=0; i<count; i++) {
		std::string testName = "Test " + std::to_string(i);
		t.handleOperation(testName, testData);
	}

	cout << "Main exits " << endl;
	
	return 0;
}
