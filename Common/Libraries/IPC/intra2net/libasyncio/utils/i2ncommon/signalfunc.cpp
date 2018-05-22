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
 * @brief implementation of wrapper and tools for signal related stuff.
 *
 * @copyright &copy; Copyright 2007-2009 by Intra2net AG
 *
 * opensource@intra2net.com
 */

#include "signalfunc.hpp"

#include <algorithm>

#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <pwd.h>
#include <grp.h>

#include "containerfunc.hpp"


namespace I2n
{
namespace SystemTools
{


namespace
{

/**
 * helper for using sigaddset() as unary function within (STL) algorithms.
 */
struct SigAddSet
{
    sigset_t *m_set;
    SigAddSet(sigset_t *set) : m_set(set) {}
    int operator() (int sig) { return sigaddset(m_set,sig); }
}; // eo struct SigAddSet



/**
 * blocks the given signals while existing.
 *
 * This class is the real (internal) implementation of the @a SignalBlocker .
 *
 * @internal This internal implementation is used to avoid including signal.h within the header file.
 * This way, we can keep the header clean; and hide all the internals in the implementation.
 */
class _ScopedSignalBlocker : public SystemTools::Detail::SObject
{
    public:
        _ScopedSignalBlocker(const std::vector<SystemTools::Signal>& sigs);
        ~_ScopedSignalBlocker();

        bool successful() const {return m_set; }

    protected:
        sigset_t m_sigset[1];
        sigset_t m_oldsigset[1];
        bool m_set;
}; // eo _ScopedSignalBlocker


/**
 * Blocks the given signals.
 *
 * Constructs a sigset from the passed signals; and calls sigprocmask to block them.
 * In case of interruption with EINTR during the call, the call is repeated some times
 * to get the desired signals blocked.
 *
 * @param sigs the vector with the siganls which should be blocked.
 */
_ScopedSignalBlocker::_ScopedSignalBlocker(const std::vector<SystemTools::Signal>& sigs)
{
    sigemptyset(m_sigset);

    // fill in signals
    std::for_each(sigs.begin(), sigs.end(), SigAddSet(m_sigset) );

    int res;
    for(int cnt=1000; cnt-->0;)
    {
        res= sigprocmask(SIG_BLOCK,m_sigset, m_oldsigset);
        if (!res || errno!=EINTR) break;
    }
    m_set= (res==0);
} // eo _ScopedSignalBlocker::_ScopedSignalBlocker()


/**
 * Unblocks the signals by restoring the original block mask.
 */
_ScopedSignalBlocker::~_ScopedSignalBlocker()
{
    int res;
    if (!m_set) return;
    for(int cnt=1000; cnt-->0;)
    {
        res= sigprocmask(SIG_SETMASK, m_oldsigset, NULL);
        if (!res || errno!=EINTR) break;
    }
} // eo _ScopedSignalBlocker::~_ScopedSignalBlocker()



} // eo namespace <anonymous>


/*************************************************************************\
\*************************************************************************/



/*
 * Signal
 */
/// @cond

const int Signal::VOID= 0;

// helper macro to import the signal nums:
#define IMPORT(sig) const int Signal::sig = SIG ## sig

IMPORT(HUP);
IMPORT(INT);
IMPORT(QUIT);
IMPORT(ILL);
IMPORT(TRAP);
IMPORT(ABRT);
IMPORT(IOT);
IMPORT(BUS);
IMPORT(FPE);
IMPORT(KILL);
IMPORT(USR1);
IMPORT(SEGV);
IMPORT(USR2);
IMPORT(PIPE);
IMPORT(ALRM);
IMPORT(TERM);
IMPORT(STKFLT);
IMPORT(CLD);
IMPORT(CHLD);
IMPORT(CONT);
IMPORT(STOP);
IMPORT(TSTP);
IMPORT(TTIN);
IMPORT(TTOU);
IMPORT(URG);
IMPORT(XCPU);
IMPORT(XFSZ);
IMPORT(VTALRM);
IMPORT(PROF);
IMPORT(WINCH);
IMPORT(POLL);
IMPORT(IO);
IMPORT(PWR);
IMPORT(SYS);

// remove helper macro
#undef IMPORT

/// @endcond

int Signal::RT(int num)
{
    return SIGRTMIN + num;
} // eo Signal::RT


/*
 * SignalCode
 */


/// @cond

// basically: define apropriate CODE macro and copy the CODE() parts from hpp:

#undef CODE
#define CODE(name) const int SignalCode::name = SI_ ## name
    CODE(USER);    CODE(QUEUE); CODE(TIMER); CODE(MESGQ);
    CODE(ASYNCIO);
#undef SIGIO
    CODE(SIGIO);
#undef CODE
#define CODE(name) const int SignalCode::ILL::name = ILL_ ## name
        CODE(ILLOPC); CODE(ILLOPN); CODE(ILLADR); CODE(ILLTRP);
        CODE(PRVOPC); CODE(PRVREG); CODE(COPROC); CODE(BADSTK);
#undef CODE
#define CODE(name) const int SignalCode::FPE::name = FPE_ ## name
        CODE(INTDIV); CODE(INTOVF); CODE(FLTDIV); CODE(FLTOVF);
        CODE(FLTUND); CODE(FLTRES); CODE(FLTINV); CODE(FLTSUB);
#undef CODE
#define CODE(name) const int SignalCode::SEGV::name = SEGV_ ## name
        CODE(MAPERR); CODE(ACCERR);
#undef CODE
#define CODE(name) const int SignalCode::BUS::name = BUS_ ## name
        CODE(ADRALN); CODE(ADRERR); CODE(OBJERR);
#undef CODE
#define CODE(name) const int SignalCode::TRAP::name = TRAP_ ## name
        CODE(BRKPT); CODE(TRACE);
#undef CODE
#define CODE(name) const int SignalCode::CHLD::name = CLD_ ## name
        CODE(EXITED); CODE(KILLED); CODE(DUMPED); CODE(TRAPPED);
        CODE(STOPPED); CODE(CONTINUED);
#undef CODE
#define CODE(name) const int SignalCode::POLL::name = POLL_ ## name
        CODE(IN); CODE(OUT); CODE(MSG); CODE(ERR); CODE(PRI); CODE(HUP);
#undef CODE
/// @endcond

/*
 * ScopedSignalBlocker
 */


/**
 * Blocks the given signal.
 * @param sig the signal which should be blocked.
 */
ScopedSignalBlocker::ScopedSignalBlocker(Signal sig)
{
    Implementation = new _ScopedSignalBlocker( TransientPushBackFiller< Signal, std::vector >()(sig) );
} // eo ScopedSignalBlocker::ScopedSignalBlocker


ScopedSignalBlocker::ScopedSignalBlocker(Signal sig1, Signal sig2)
{
    Implementation = new _ScopedSignalBlocker( TransientPushBackFiller< Signal, std::vector >()(sig1)(sig2) );
} // eo ScopedSignalBlocker::ScopedSignalBlocker


ScopedSignalBlocker::ScopedSignalBlocker(Signal sig1, Signal sig2, Signal sig3)
{
    Implementation = new _ScopedSignalBlocker( TransientPushBackFiller< Signal, std::vector >()(sig1)(sig2)(sig3) );
} // eo ScopedSignalBlocker::ScopedSignalBlocker


/**
 * Blocks the given signals.
 * @param sigs vector with the signals which should be blocked.
 */
ScopedSignalBlocker::ScopedSignalBlocker(const std::vector<Signal>& sigs)
{
    Implementation = new _ScopedSignalBlocker( sigs );
} // eo ScopedSignalBlocker::ScopedSignalBlocker


/**
 * Unblocks the signals by restoring the previous blocking list.
 */
ScopedSignalBlocker::~ScopedSignalBlocker()
{
    if (Implementation)
    {
        delete Implementation;
        Implementation= NULL;
    }
} // eo ScopedSignalBlocker::ScopedSignalBlocker



/*
** signal handling
*/

typedef std::map< int, struct sigaction > SignalActionMap;

namespace {

SignalActionMap original_signal_action;

} // eo namespace <anonymous>


/**
 * @brief installs a new signal action.
 * @param sig the signal
 * @param new_action the new signal action.
 * @return @a true iff the new signal action was succesfully installed.
 *
 * Remembers the original value of the signal handler for later restoring.
 */
bool install_signal_handler(
    Signal sig,
    struct sigaction& new_action
)
{
    struct sigaction old_action[1];
    int signum= sig;
    int res= ::sigaction(signum, &new_action, old_action);
    if (0 == res)
    {
        SignalActionMap::iterator it= original_signal_action.find(signum);
        if (it == original_signal_action.end())
        {
            original_signal_action[signum] = *old_action;
        }
    }
    else if (res > 0)
    {
        // some glibc's seem to return the errno instead of storing
        // it in the appropriate var... *sigh*
        errno = res;
    }
    return (0 == res);
} // eo install_signal_handler(int,sigaction&)


/**
 * @brief installs a simple signal handler.
 * @param sig the signal.
 * @param handler pointer to the signal handler.
 * @return @a true iff the handler was successfully installed.
 *
 * Remembers the original value of the signal handler for later restoring.
 */
bool install_signal_handler(
    Signal sig,
    void(*handler)(int)
)
{
    struct sigaction new_action;
    new_action.sa_handler= handler;
    sigemptyset( &new_action.sa_mask );
    new_action.sa_flags= 0;
    new_action.sa_restorer= NULL;
    return install_signal_handler(sig, new_action);
} // eo install_signal_handler(signum,void(*)(int))



/**
 * @brief installs a signal action handler.
 * @param sig the signal
 * @param handler pointer to the signal action handler.
 * @return @a true iff the action handler was successfully installed.
 *
 * Remembers the original value of the signal handler for later restoring.
 */
bool install_signal_handler(
    Signal sig,
    void(*handler)(int,struct siginfo*,void*)
)
{
    struct sigaction new_action;
    new_action.sa_sigaction= (void (*)(int, siginfo_t*, void*))(handler);
    sigemptyset( &new_action.sa_mask );
    new_action.sa_flags= SA_SIGINFO;
    new_action.sa_restorer= NULL;
    return install_signal_handler(sig, new_action);
} // eo install_signal_handler(signum,void(*)(int,siginfo_t*,void*))


/**
 * @brief ignores a signal.
 * @param sig the signal
 * @return @a true iff the ignore handler was successfully installed.
 *
 * Remembers the original value of the signal handler for later restoring.
 */
bool ignore_signal(Signal sig)
{
    return install_signal_handler(sig, SIG_IGN );
} // eo ignore_signal(Signal)


/**
 * @brief enables the default signal handler.
 * @param sig the signal
 * @return @a true iff the default handler was successfully installed.
 *
 * Remembers the original value of the signal handler for later restoring.
 */
bool install_default_signal_handler(Signal sig)
{
    return install_signal_handler(sig, SIG_DFL );
} // eo install_default_signal_handler(Signal)


/**
 * @brief restores a signal handle to its original value.
 * @param sig the signal.
 * @return @a true iff the handler was sucessfully restored.
 */
bool restore_signal_handler(Signal sig)
{
    int signum= sig;
    SignalActionMap::iterator it= original_signal_action.find(signum);
    int res= -1;
    if (it != original_signal_action.end())
    {
        res= ::sigaction(signum, &(it->second), NULL);
        if (0 == res)
        {
            original_signal_action.erase(it);
        }
    }
    return (0 == res);
} // eo restore_signal_handler



/**
 * @brief convenience function; send's a signal to a process.
 * @param pid PID of the process which should recive the signal.
 * @param signal the signal to send.
 * @return @a true iff sending of the signal succeeded.
 */
bool send_signal( pid_t pid, Signal signal)
{
    return ::kill(pid, signal.Value) == 0;
} // eo send_signal(pid_t,Signal)



} // eo namespace SystemTools
} // eo namespace I2n
