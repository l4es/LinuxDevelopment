/*
The software in this package is distributed under the GNU General
Public License version 2 (with a special exception described below).

A copy of GNU General Public License (GPL) is included in this distribution,
in the file COPYING.GPL.

As a special exception, if other files instantiate templates or use macros
or inline functions from this file, or you compile this file and link it
with other works to produce a work based on this file, this file
does not by itself cause the resulting work to be covered
by the GNU General Public License.

However the source code for this file must still be made available
in accordance with section (3) of the GNU General Public License.

This exception does not invalidate any other reasons why a work based
on this file might be covered by the GNU General Public License.
*/
/** @file
 *
 * (c) Copyright 2007-2008 by Intra2net AG
 */

//#define NOISEDEBUG

#include "async_process.hpp"

#include <iterator>
#include <algorithm>

#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>

//#include <filefunc.hxx>


#ifdef NOISEDEBUG
#include <iostream>
#include <iomanip>
#define DOUT(msg) std::cout << msg << std::endl
#define FODOUT(obj,msg) std::cout << typeid(*obj).name() << "[" << obj << "]:" << msg << std::endl
#define ODOUT(msg) std::cout << typeid(*this).name() << "[" << this << "]:" << msg << std::endl
#else
#define DOUT(msg) do {} while (0)
#define FODOUT(obj,msg) do {} while (0)
#define ODOUT(msg) do {} while (0)
#endif


namespace
{

using namespace AsyncIo;

/**
 * local configuration values
 */
namespace config
{

   /// the capacity of the child status list (/ vector)
   const unsigned int pid_pool_capacity= 512;

} // eo namespace config



/// the previous handler for the child signal (SIGCHLD)
void (*oldChildHandler)(int) = NULL;

/// indicates if we installed our own signal handler or not
bool installedChildHandler = false;

/// method pointer for activating process manager
void (ProcessManager::*_activate_manager)();

PidStateList pending_pid_states;


/**
 * signal handler for child signal (SIGCHLD)
 * @param sig the signal number as provided by the OS
 */
void handleSigChild(int sig)
{
   int status;
   pid_t pid;
   while ( (pid = waitpid(-1,&status,WNOHANG)) > 0)
   {
      pending_pid_states.push_back( PidStatePair(pid,status) );
   }
   if (_activate_manager)
   {
      // tricky way to access a protected method without being a (official) friend:
      ( ProcessManager::getInstance()->*_activate_manager)();
   }
   //TODO: ?
   signal(sig,handleSigChild);
} // eo handleSigChild


namespace process
{

typedef std::pair<pid_t, ProcessImplementation*> PidProcPair;
typedef std::list< PidProcPair > PidProcList;


template< typename F, typename S >
struct CmpFirst
{
   F _f;
   CmpFirst ( F f ) : _f(f) {}
   bool operator () ( const std::pair<F,S>& v ) const { return v.first == _f; }
}; // eo struct CmpFirst


std::list<ProcessImplementation*> g_process_list;
PidProcList g_pid_list;


void addProcessInstance( ProcessImplementation* obj )
{
   g_process_list.push_back(obj);
} // eo addProcessInstance(ProcessImplementation*)


void removeProcessInstance( ProcessImplementation* obj )
{
   // remove obj from list
   g_process_list.remove(obj);
   // clear pointers in pid list
   for(PidProcList::iterator it= g_pid_list.begin();
      it != g_pid_list.end();
      ++it)
   {
      if (it->second == obj)
      {
         it->second= NULL;
      }
   }
} // eo removeProcessInstance(ProcessImplementation*)


void addChildProcess( pid_t pid, ProcessImplementation* obj)
{
   g_pid_list.push_back ( PidProcPair(pid,obj) );
} // eo addChildProcess(pid_t,ProcessImplementation*)


void removeChildProcess ( pid_t pid, ProcessImplementation* obj)
{
   PidProcList::iterator it= std::find(
      g_pid_list.begin(), g_pid_list.end(),
      PidProcPair(pid,obj));
   if (it != g_pid_list.end())
   {
      g_pid_list.erase(it);
   }
} // eo removeChildProcess(pid_t,ProcessImplementation*)


bool findChildProcess ( pid_t pid, ProcessImplementation* & obj )
{
   PidProcList::iterator it = std::find_if(
      g_pid_list.begin(), g_pid_list.end(),
      CmpFirst<pid_t,ProcessImplementation*>(pid) );
   if (it == g_pid_list.end())
   {
      return false;
   }
   obj = it->second;
   return true;
} // eo findChildProcess(pid_t,ProcessImplementation*&)


} // eo namespace process





/*
** misc tools
*/


/**
 * convenience tool for closing file descriptors...
 */
struct FdCloser
{
   int m_fd;

   FdCloser(int fd=-1) : m_fd(fd) {}

   ~FdCloser()
   {
      if (m_fd >= 0) ::close(m_fd);
   }

   void release() { m_fd= -1; }

}; // eo struct FdCloser



} // eo namespace <anonymous>


namespace AsyncIo
{


/*
 * global functions
 */

/**
 * installs the handler for the child signal (SIGCHLD).
 * Installing this handler is mandatory for the process subsystem to work correctly.
 * @return @a true iff the child handler is successfully installed.
 */
bool installChildHandler()
{
   if (installedChildHandler)
   {
      // already installed
      return true;
   }
   if (! ProcessManager::getInstance() )
   {
      // we need an instance of the process manager
      return false;
   }
   pending_pid_states.reserve( config::pid_pool_capacity );
   oldChildHandler = signal( Signal::CHLD, handleSigChild );
   if (oldChildHandler == SIG_ERR)
   {
      oldChildHandler = NULL;
      return false;
   }
   installedChildHandler = true;
   return true;
} // eo installChildHandler


/**
 * uninstalls the child handler.
 * @return @a true iff the old child handler is reestablished.
 */
bool restoreChildHandler()
{
   if (!installedChildHandler)
      return false;

   void(*res)(int) = signal( Signal::CHLD, oldChildHandler );
   if (res == SIG_ERR)
      return false;

   oldChildHandler = NULL;
   installedChildHandler = false;

   return true;
} // eo restoreChildHandler




/*
 * Implementation of ProcessImplementation
 */

IOImplementation2* ProcessImplementation::_StderrOnStdout   = ((IOImplementation2*) 1);
IOImplementation2* ProcessImplementation::_UseParentsStderr = ((IOImplementation2*) 0);


/**
 * @brief constructor for the process implementation.
 *
 * the constructor takes the path to the executable and (initial) cli arguments.
 *
 * @param path path to the executable.
 * @param args initial command line arguments.
 */
ProcessImplementation::ProcessImplementation(
   const std::string& path,
   const std::vector<std::string>& args
   )
: IOImplementation(-1,-1)
, m_path(path)
, m_nice_inc(0)
, m_create_new_session(false)
, m_pid(0)
, m_state(ProcessState::stopped)
, m_exit_code(0)
{
   m_args.push_back(path);
   std::copy( args.begin(), args.end(), std::back_inserter(m_args) );
   process::addProcessInstance(this);
} // eo ProcessImplementation::ProcessImplementation(const std::string&)


ProcessImplementation::~ProcessImplementation()
{
   if (m_pid > 0 && m_state!=ProcessState::stopped)
   {
      stopProcess(true);
   }
   process::removeProcessInstance(this);
} // eo ProcessImplementation::~ProcessImplementation()


void ProcessImplementation::close(Direction direction)
{
   inherited::close(direction);
   if (!inherited::opened() &&  (m_state != ProcessState::stopped) )
   {
      stopProcess(false);
   }
} // eo ProcessImplementation::close(Direction)


/**
 * returns an object for adding new arguments to the argument list.
 * @return the adder object.
 */
PushBackFiller<std::string, std::vector > ProcessImplementation::getArgAdder()
{
   return PushBackFiller<std::string, std::vector >(m_args);
} // eo ProcessImplementation::getArgAdder()


/**
 * @brief set if the process should create a new session when started.
 * @param enable determine if the process should start a new session.
 * @return @a true iff the value of enable was accepted.
 *
 * If the process is already running, a new value is not accepted.
 */
bool ProcessImplementation::setCreateNewSession( bool enable )
{
   if (m_state != ProcessState::stopped and enable != m_create_new_session)
   {
      return false;
   }
   m_create_new_session= enable;
   return true;
} // eo ProcessImplementation::setCreateNewSession(bool);


/**
 * @brief sets a new nice increment.
 * @param nice the desired nice increment.
 * @return @a true if the value was accepted and - in case the process was already started -
 * the nice value was successfully changed.
 */
bool ProcessImplementation::setNice(int nice)
{
   errno= 0;
   if (m_state != ProcessState::stopped)
   {
      int delta= m_nice_inc + nice;
      m_nice_inc= nice;
      int res= ::nice(delta);
      if (res == -1 and  errno !=0 )
      {
         return false;
      }
   }
   else
   {
      m_nice_inc = nice;
   }
   return true;
} // eo ProcessImplementation::setNice(int)


/**
 * @brief sets the work dir the process should be started with.
 * @param workdir the workdir
 * @return @a true if the new workdir was accepted.
 *
 * The method will return @a false if the process is already started.
 * The workdir can only be set before the process is started.
 */
bool ProcessImplementation::setWorkDir(const std::string& workdir)
{
   if ( m_state != ProcessState::stopped and workdir != m_workdir)
   {
      return false;
   }
   if (not workdir.empty())
   {
       Utils::FileStat stat(workdir);
       if (not stat or not stat.is_directory())
       {
           return false;
       }
   }
   m_workdir= workdir;
   return true;
} // eo ProcessImplementation::setWorkDir(const std::string&)


/**
 * @brief sets new arguments for the process (the path to the binary is kept).
 *
 * @param args the new cli arguments for the subprocess (replacing the old ones).
 */
void ProcessImplementation::resetArgs( const std::vector< std::string >& args )
{
   if (m_args.size() > 1)
   {
      m_args.erase( ++m_args.begin(), m_args.end());
   }
   std::copy( args.begin(), args.end(), std::back_inserter(m_args) );
} // eo ProcessImplementation::resetArgs(const std::vectors< std::string >&)


/**
 * starts the new process.
 * provides pipes for sending data to/ receiving data from the new process.
 * Basically forks and execs the new process.
 *
 * @param stderr if not NULL the given object will be connected to stderr of the new process.
 *  The object can then be used for reading the data from the process' stderr; but cannot be written to.
 * (The object will be closed if it was open).
 * If the constant @a ProcessImplementation::StderrOnStdout is passed then stderr of the new process will
 * be written to the same channel as stdout (i.e. can be read from the process class instance like the
 * normal output).
 * If NULL then the stderr channel from the parent process will also be used by the child.
 * @return @a true iff the new subprocess started.
 */
bool ProcessImplementation::startProcess( IOImplementation2 *stderr )
{
   bool stderr2stdout= false;
   m_errno = 0;
   m_input_buffer.clear();
   if (m_pid > 0 && m_state != ProcessState::stopped)
   {
      // process still/already running...
      return false;
   }

   Utils::FileStat stat(m_path);
   if (! stat.is_exec_file())
   {
       // command to execute does not exist or is not executable
       return false;
   }

   m_exit_code= 0;

   if (stderr == _StderrOnStdout)
   {
      stderr2stdout= true;
      stderr= NULL;
   }

   int to_process_pipe[2];
   int from_process_pipe[2];
   int from_process_stderr_pipe[2]= { -1, -1 };

   if ( ::pipe(to_process_pipe) )
   {
      m_errno= errno;
      return false;
   }
   FdCloser closeTo0( to_process_pipe[0] );
   FdCloser closeTo1( to_process_pipe[1] );
   if ( ::pipe (from_process_pipe) )
   {
      m_errno= errno;
      return false;
   }
   FdCloser closeFrom0( from_process_pipe[0] );
   FdCloser closeFrom1( from_process_pipe[1] );
   if (stderr)
   {
      if (stderr->opened()) stderr->close();
      if ( ::pipe (from_process_stderr_pipe) )
      {
         m_errno= errno;
         return false;
      }
   }
   FdCloser closeFromErr0( from_process_stderr_pipe[0] );
   FdCloser closeFromErr1( from_process_stderr_pipe[1] );

   m_pid = ::fork();

   if ( m_pid == (pid_t)-1 )
   {
      m_errno= errno;
      m_pid= 0;
      // error; something went wrong
      return false;
   }
   else if (m_pid > 0)
   {
      // we are in the parent part

      // keep the fd's we need and (later) close the other ones:
      closeTo1.release(); // don't close this fd!
      setWriteFd(to_process_pipe[1]);
      closeFrom0.release(); // don't close this fd!
      setReadFd(from_process_pipe[0]);

      if (stderr)
      {
         closeFromErr0.release(); // don't close this fd!
         stderr->setReadFd(from_process_stderr_pipe[0]);
      }

      m_state= ProcessState::running;
      process::addChildProcess(m_pid,this);
      DOUT(" started child with pid " << m_pid);
      return true;
   }
   else // pid > 0
   {
      // we are in the child part

      // dup the fd's for stdin/-out/-err into place:
      ::dup2(to_process_pipe[0],0);
      ::dup2(from_process_pipe[1],1);
      if (stderr)
      {
         ::dup2(from_process_stderr_pipe[1],2);
         ::close(from_process_stderr_pipe[0]); ::close(from_process_stderr_pipe[1]);
      }
      else if (stderr2stdout)
      {
         ::dup2(from_process_pipe[1],2);
      }
      // close what we don't need:
      ::close(to_process_pipe[0]); ::close(to_process_pipe[1]);
      ::close(from_process_pipe[0]); ::close(from_process_pipe[1]);

      // set workdir if requested:
      if (not m_workdir.empty())
      {
         int r= ::chdir( m_workdir.c_str() );
         if (r !=0 )
         {
            //TODO?
            _exit(255);
         }
      }

      //
      // collect args:
      char **argv= new char*[m_args.size()+1];
      int i=0;
      for(std::vector<std::string>::iterator it= m_args.begin();
         it != m_args.end();
         ++it,++i)
      {
         argv[i]= strdup( it->c_str() );
      }
      argv[i]= NULL;
      // update nice level:
      if (m_nice_inc)
      {
         nice(m_nice_inc);
      }
      // create a new session id if requested:
      if (m_create_new_session)
      {
         setsid();
      }
      // execute:
      execv(m_path.c_str(), argv);
      // exit if exec failed
      _exit(255);
      //cleanup! ... just joking; we exec or we exit, in either case the system cleans
      // everything which needs to be cleaned up.
      // Using _exit instead of exit, it will not clean up the clone of parent's memory
      //   in case of failure to exec m_path
   }
   return false; // keep the compiler happy...
} // eo ProcessImplementation::startProcess()


/**
 * convenience method for starting the child process.
 * This method uses predefined enum values for the stderr handling mode.
 *
 * @param stderr_mode the desired stderr mode.
 * @return @a true iff the child process was created.
 */
bool ProcessImplementation::startProcess( ProcessImplementation::StderrMode stderr_mode )
{
   switch (stderr_mode)
   {
      case UseParentsStderr:
         return startProcess( _UseParentsStderr );

      case StderrOnStdout:
         return startProcess( _StderrOnStdout );
   }
   return false;
}; // eo ProcessImplementation::startProcess(ProcessImplementation::StderrMode)


/**
 * stops the process.
 *
 * @todo think about a more intelligent handling...
 */
void ProcessImplementation::stopProcess(bool force)
{
   // TODO: do it somewhat more intelligent?!
   if (force)
   {
      kill(Signal::KILL);
      //TODO: set running state?
   }
   else
   {
      kill(Signal::TERM);
   }
} // eo ProcessImplementation::stop(bool)



/**
 * sends a signal to the child process.
 * @param signal the Signal which should be send.
 * @return @a true if the signal was sent; @a false if an error occured.
 */
bool ProcessImplementation::kill(Signal signal)
{
   m_errno = 0;
   if (m_pid == 0 || m_pid == (pid_t)-1)
   {
      m_errno= ESRCH;
      return false;
   }
   int res = ::kill(m_pid, signal);
   if (res < 0)
   {
      m_errno= errno;
      return false;
   }
   if (signal == Signal::CONT && m_state == ProcessState::suspended)
   {
      m_state = ProcessState::running;
   }
   return true;
} // eo ProcessImplementation::kill(Signal)



/**
 * set a new child state with information gobbled by the child signal handler.
 *
 * @note This method should only be called by the process manager!
 *
 * @param pid the pid of the child process.
 * @param status the new status value (as delivered by waitpid())
 */
void ProcessImplementation::setChildState(pid_t pid, int status)
{
   DOUT("setChildState("<<pid<<","<<status<<")   pid="<<m_pid);
   if (pid != m_pid)
   {
      // old child... ignore!
      return;
   }
   if (WIFSTOPPED(status))
   {
      DOUT("stopped");
      // stopped:
      int stopsignal = WSTOPSIG(status);
      // make stop signal available in exit_code:
      m_exit_code= (stopsignal << 8);
      m_state= ProcessState::suspended;
      return;
   }
#ifdef WIFCONTINUED
   if (WIFCONTINUED(status))
   {
      DOUT("continued");
      // continued after a stop:
      m_state= ProcessState::running;
      return;
   }
#endif
   if (WIFEXITED(status))
   {
      DOUT("normal exit");
      //normal exit:
      m_exit_code= (0xff & WEXITSTATUS(status));
      m_pid= 0;
      close(Direction::out);
      m_state= ProcessState::stopped;
      m_signal_terminated();
      return;
   }
   if (WIFSIGNALED(status))
   {
      DOUT("signaled stop");
      // exit by signal:
      int termsignal = WTERMSIG(status);
      // make term signal available in exit code (normal exit codes are only 8 bit)
      m_exit_code = (termsignal << 8);
      m_pid= 0;
      close(Direction::out);
      m_state= ProcessState::stopped;
      m_signal_terminated();
      return;
   }
   // this point should never be reached...!!
} // eo ProcessImplementation::setChildState(pid_t,int)


/*
 * implementation of ProcessManager
 */

/// the instance of the process manager (highlander; there can be only one!)
ProcessManager* ProcessManager::the_instance= NULL;


ProcessManager::ProcessManager()
{
   setWhenTime(0);
} // eo ProcessManager::ProcessManager


/**
 * delivers the process manager instance (generate if it doesn't exist)
 * @return the process manager instance
 */
ProcessManager* ProcessManager::getInstance()
{
   if (! the_instance)
   {
      the_instance = new ProcessManager();
      _activate_manager = &ProcessManager::activateMe;
   }
   return the_instance;
} // eo ProcessManager::getInstance


/**
 * activate the timer so it's handled by the next backend cycle
 */
void ProcessManager::activateMe()
{
   setWhenTime(0);
   activate();
} // eo ProcessManager::activateMe


/**
 * real work is done here.
 * Processes the information collected by the child signal handler.
 */
void ProcessManager::execute()
{
   PidStateList pid_state_list;
   {
      // block child signals (within this scope)
      ScopedSignalBlocker blocker( Signal::CHLD );
      // and now fetch the list of pending information
      // (simply swap with our local empty list)
      std::swap(pid_state_list, pending_pid_states);
      // reserve the desired (minimum) capacity
      pending_pid_states.reserve( config::pid_pool_capacity );
   }
   ODOUT("exec, " << pid_state_list.size() << " entries");

   // interpret states:
   for(PidStateList::iterator it = pid_state_list.begin();
      it != pid_state_list.end();
      ++it)
   {
      pid_t pid  = it->first;
      int status = it->second;
      ODOUT("  pid=" << pid << ", status=" << status);
      ProcessImplementation *process_obj;
      if (process::findChildProcess(pid,process_obj))
      {
         ODOUT("  local managed child,  process_obj="<< process_obj);
         // pid found in list:
         if (!WIFSTOPPED(status)
#ifdef WIFCONTINUED
            && !WIFCONTINUED(status)
#endif
            )
         {
            // take it from list if the child exited:
            process::removeChildProcess(pid,process_obj);
         }
         if (process_obj)
         {
            // give the process object a chance to handle the state change:
            process_obj->setChildState(pid, status);
         }
      }
      else
      {
         ODOUT("foreign child");
         // pid not found in list:
         /* NOTE: in a non threaded environment this pid must be from a child process which is not
            managed by this process classes; since this method is called after all setup of a child process
            is done (; especially entering the new child pid into our internal lists).
         */
         m_foreign_pid_states.push_back(*it);
      }
   }

   // handle the foreign childs:
   {
      /* idea:
       * fetch a (pid,status) from the list, erase it (to avoid reentrance problems)
       * and fire the signal. If someone forks childs outside this module then he can
       * connect to the signal and receive all necessary status information gobbled by
       * our child handler.
       */
      while (! m_foreign_pid_states.empty())
      {
         PidStateList::iterator it= m_foreign_pid_states.begin();
         pid_t pid  = it->first;
         int status = it->second;
         m_foreign_pid_states.erase(it);
         m_foreign_child_state_changed_signal(pid,status);
      }
   }

} // eo ProcessManager::execute


} // eo namespace AsyncIo
