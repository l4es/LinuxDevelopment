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
 * simple process handling based on simple io classes.
 *
 * (c) Copyright 2007-2008 by Intra2net AG
 */

#ifndef __ASYNC_PROCESS_HPP__
#define __ASYNC_PROCESS_HPP__

#include <vector>
#include <utility>

#include <sys/types.h>

#include <asyncio_containerfunc.hpp>
#include <asyncio_signalfunc.hpp>
#include "async_io.hpp"


namespace AsyncIo
{

using SystemTools::Signal;
using SystemTools::ScopedSignalBlocker;


class ProcessManager;


typedef std::pair< pid_t, int >      PidStatePair;
typedef std::vector< PidStatePair >  PidStateList;


/**
 * represents process states.
 */
struct ProcessState
{
   enum _ProcessState
   {
      stopped = 0,
      running,
      suspended
   }; // eo enum _ProcessState

   _ProcessState m_state;

   ProcessState(_ProcessState _state = stopped) : m_state(_state) {}

   operator _ProcessState() const { return m_state; }
}; // eo struct ProcessState


/**
 * specialisation of the io implementation class which fork/exec's a subprocess and
 * connects with the new child's stdin/stdout.
 *
 * @note the signal @a IOImplementation::m_signal_eof of the base class can be used to detect when the
 * new child closes it's stdout (which usually means that the child ended).
 */
class ProcessImplementation : public IOImplementation
{
      typedef IOImplementation inherited;

      friend class ProcessManager;

   public:

      enum StderrMode
      {
         /// "magic" constant to pass to start() when childs stderr should be the same as parents stderr.
         UseParentsStderr= 0,
         /// "magic" constant to pass to start() when stderr should be the same as stdout for the new process.
         StderrOnStdout
      }; // eo enum StderrMode

   public:
      ProcessImplementation(
         const std::string& path,
         const std::vector<std::string>& args = std::vector<std::string>());
      virtual ~ProcessImplementation();

      virtual void close(Direction direction = Direction::both);

      virtual bool startProcess( IOImplementation2* stderr );
      bool startProcess( StderrMode stderr_mode = UseParentsStderr );

      virtual void stopProcess(bool force=false);

      PushBackFiller<std::string, std::vector > getArgAdder();

      bool setCreateNewSession( bool enable= true);

      bool setNice(int nice);

      bool setWorkDir(const std::string& workdir);

      void resetArgs( const std::vector< std::string >& args = std::vector< std::string >() );

      /// returns the current process state
      ProcessState processState() const { return m_state; }

      ///returns the exit code of the process (if in stopped state)
      int exitCode() const { return m_exit_code; }

      bool kill(const Signal signal);

      /// returns the pid of the child process
      pid_t pid() const { return m_pid; }

   protected:

      void setChildState(pid_t pid, int status);

   protected:
      /// the path to the binary
      std::string m_path;
      /// argument list (starting with argv0, usually the name of the binary)
      std::vector<std::string> m_args;
      /// increment of the nice level when the new child is started
      int  m_nice_inc;
      /// determines if the child should start a new session.
      bool m_create_new_session;
      /// determines the workdir where the child process should be started with.
      std::string m_workdir;

      /// the pid of the child process
      pid_t m_pid;
      /// the state of the child process
      ProcessState m_state;
      /// the exit code of the child (-1 if not available yet)
      int  m_exit_code;

      /// signal which is fired when the child terminated
      SignalType m_signal_terminated;


      /// "magic" constant to pass to start() when childs stderr should be the same as parents stderr.
      static IOImplementation2* _UseParentsStderr;
      /// "magic" constant to pass to start() when stderr should be the same as stdout for the new process.
      static IOImplementation2* _StderrOnStdout;

   private:

}; // eo class ProcessImplementation


/**
 * manages overall process related stuff.
 *
 * @note this class is implemented as a singleton.
 * @note this class uses the io timer interface to be called within the backend loops when necessary.
 */
class ProcessManager : public TimerBase
{
   public:

      static ProcessManager* getInstance();

   protected:
      ProcessManager();
      ProcessManager(const ProcessManager&);

      virtual void execute();

      void activateMe();

   public:

      /**
       * the signal which is fired when waitpid() returns a status for a child process
       * which is not managed by this process subsystem.
       * Another module which forks child processes can connect to this signal to receive
       * the information when these child processes are terminated.
       */
      boost::signal<void(pid_t,int)> m_foreign_child_state_changed_signal;

   protected:

      static ProcessManager *the_instance;

      PidStateList  m_foreign_pid_states;

   private:
};


bool installChildHandler();
bool restoreChildHandler();


} // eo namespace AsyncIo

#endif
