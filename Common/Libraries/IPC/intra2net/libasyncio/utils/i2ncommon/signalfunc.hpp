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
 * @brief provides wrapper and tools for signal related stuff.
 *
 *
 * @copyright &copy; Copyright 2007-2008 by Intra2net AG
 * @contact opensource@intra2net.com
 *
 * @bug 
 * Although most stuff should work under most POSIX like systems;
 * some funcs might be very linux related.
 * (But at least we use that lib currently under linux only.)
 */

#ifndef _I2N_SIGNALFUNC_HPP_
#define _I2N_SIGNALFUNC_HPP_

#include <vector>

// with pain in the stomach; the following include was added....:
// (since these includes a lot of #define's... what we usually don't want to have in C++ headers...)
#include <sys/types.h>

extern "C"
{

struct siginfo;

}


namespace I2n
{
namespace SystemTools
{


namespace Detail
{

/**
 * base class for internal implementation classes.
 */
class SObject
{
    public:
        virtual ~SObject() {}
}; // eo class SObject

} // eo namespace Detail

/**
 * @brief representation of system signal.
 *
 * This struct also provides constants for the usual system signals; so it is not necessary to include signal.h
 * for obtaining the constants.
 *
 * Due to an appropriate cast operator instances of the class can be used in all places where
 * a plain signal (i.e. an int) is expected.
 */
struct Signal
{
    static const int VOID;
/// @cond

#define SIG(s) static const int s
    SIG(HUP);   SIG(INT);   SIG(QUIT);  SIG(ILL);   SIG(TRAP);
    SIG(ABRT);  SIG(IOT);   SIG(BUS);   SIG(FPE);   SIG(KILL);
    SIG(USR1);  SIG(SEGV);  SIG(USR2);  SIG(PIPE);  SIG(ALRM);
    SIG(TERM);  SIG(STKFLT); SIG(CLD);  SIG(CHLD);  SIG(CONT);  SIG(STOP);
    SIG(TSTP);  SIG(TTIN);  SIG(TTOU);  SIG(URG);   SIG(XCPU);
    SIG(XFSZ);  SIG(VTALRM); SIG(PROF); SIG(WINCH); SIG(POLL);
    SIG(IO);    SIG(PWR);   SIG(SYS);
#undef SIG

/// @endcond

    static int RT(int num=0);

    int Value;

    Signal(int signal) : Value(signal) {}

    operator int () const { return Value; }
}; // eo struct Signal


/**
 * @brief representation of signal codes.
 *
 * This struct provides constants for signal codes; so it is not necessary to include signal.h
 * for obtaining the constants.
 *
 * Due to an appropriate cast operator instances of the class can be used in all places where
 * a plain signal code (i.e. an int) is expected.
 */
struct SignalCode
{


/// @cond
#define CODE(name) static const int name
    CODE(USER);    CODE(QUEUE); CODE(TIMER); CODE(MESGQ);
    CODE(ASYNCIO);
#undef SIGIO
    CODE(SIGIO);
/// @endcond

    /**
     * @brief contains the codes for signal SIGILL
     */
    struct ILL
    {
        /// @cond
        CODE(ILLOPC); CODE(ILLOPN); CODE(ILLADR); CODE(ILLTRP);
        CODE(PRVOPC); CODE(PRVREG); CODE(COPROC); CODE(BADSTK);
        /// @endcond
    }; // eo struct ILL;


    /**
     * @brief contains the codes for signal SIGFPE
     */
    struct FPE
    {
        /// @cond
        CODE(INTDIV); CODE(INTOVF); CODE(FLTDIV); CODE(FLTOVF);
        CODE(FLTUND); CODE(FLTRES); CODE(FLTINV); CODE(FLTSUB);
        /// @endcond
    }; // eo struct FPE


    /**
     * @brief contains the codes for signal SIGSEGV
     */
    struct SEGV
    {
        /// @cond
        CODE(MAPERR); CODE(ACCERR);
        /// @endcond
    }; // eo struct SEGV


    /**
     * @brief contains the codes for signal SIGBUS
     */
    struct BUS
    {
        /// @cond
        CODE(ADRALN); CODE(ADRERR); CODE(OBJERR);
        /// @endcond
    }; // eo struct BUS


    /**
     * @brief contains the codes for signal SIGTRAP
     */
    struct TRAP
    {
        /// @cond
        CODE(BRKPT); CODE(TRACE);
        /// @endcond
    }; // eo struct TRAP


    /**
     * @brief contains the codes for signal SIGCHLD
     */
    struct CHLD
    {
        /// @cond
        CODE(EXITED); CODE(KILLED); CODE(DUMPED); CODE(TRAPPED);
        CODE(STOPPED); CODE(CONTINUED);
        /// @endcond
    }; // eo struct CHLD


    /**
     * @brief contains the codes for signal SIGPOLL
     */
    struct POLL
    {
        /// @cond
        CODE(IN); CODE(OUT); CODE(MSG); CODE(ERR); CODE(PRI); CODE(HUP);
        /// @endcond
    }; // eo strcut POLL

#undef CODE

    int TheCode;

    SignalCode(int code) : TheCode(code) {}

    operator int () const { return TheCode; }
}; // eo SignalCode




/**
 * @brief helper for blocking a (or some) signal(s) during an operation.
 *
 * This class blocks the given signals when constructed and resets the original block mask
 * when destructed.
 */
class ScopedSignalBlocker
{
    public:
        ScopedSignalBlocker(Signal sig);
        ScopedSignalBlocker(Signal sig1, Signal sig2);
        ScopedSignalBlocker(Signal sig1, Signal sig2, Signal sig3);
        ScopedSignalBlocker(const std::vector<Signal>& sigs);
        virtual ~ScopedSignalBlocker();

    private:
        Detail::SObject* Implementation;

}; // eo class ScopedSignalBlocker




bool install_signal_handler(
    Signal sig,
    void(*handler)(int)
);

bool install_signal_handler(
    Signal sig,
    void(*handler)(int,struct siginfo*,void*)
);

bool ignore_signal(Signal sig);
bool install_default_signal_handler(Signal sig);

bool restore_signal_handler(Signal sig);


bool send_signal( pid_t pid, Signal signal);


} // eo namespace SysTools
} // eo namespace I2n

#endif
