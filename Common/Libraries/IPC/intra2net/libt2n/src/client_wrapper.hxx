/*
Copyright (C) 2006 by Intra2net AG - Gerd v. Egidy

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
#ifndef __LIBT2N_CLIENT_WRAPPER
#define __LIBT2N_CLIENT_WRAPPER

#include <functional>

#include <boost/config.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/preprocessor.hpp>

#include <client.hxx>
#include <command_client.hxx>

#ifndef T2N_SINGLETON_WRAPPER_MAX_ARGS
#define T2N_SINGLETON_WRAPPER_MAX_ARGS 9
#endif

namespace libt2n
{

/** @brief provide access to client_connection singletons and call-handling function

    This is an abstact base class for use with T2nSingletonWrapper. It provides access
    to the singleton-client_connection and offers a handle-method which is used for
    every call to a method on a t2n-server.
*/
class ConnectionWrapper
{
    private:
        long long command_timeout_usec;
        long long hello_timeout_usec;

    protected:
        log_level_values log_level;
        std::ostream *logstream;
        void set_logging_on_connection(client_connection& c);

    public:
        ConnectionWrapper()
            : command_timeout_usec(command_client::command_timeout_usec_default),
              hello_timeout_usec(command_client::hello_timeout_usec_default),
              log_level(none),
              logstream(NULL)
            { }

        virtual ~ConnectionWrapper()
            { }

        /** @brief return active connection, create new one if not existing

            Return a pointer to an active client_connection. Use detail-data
            stored within the derived class to create a new connection if needed,
            otherwise return an alredy active connection. The derived class has
            to take care of destroying the connection when not needed anymore.
        */
        virtual client_connection* get_connection()=0;

        /** @brief this function is called on every execution of a method on a server

            @param stubBase pointer to the command_client executing the call
            @param f boost::function object containing the method to call and all parameters
            @retval true if the call was successful and the original return-value of the called function can be used
                    false if T2nSingletonWrapper has to create a return-value with the default-constructor

            T2nSingletonWrapper will call this function on every execution of a server-side
            method. This version will just call the function without any special treatment.
            You can overload this function to implement different error handling strategies.
        */
        virtual bool handle(command_client* stubBase, boost::function< void() > f)
        {
            f();
            return true;
        }

        long long get_command_timeout_usec(void)
            { return command_timeout_usec; }

        void set_command_timeout_usec(long long _command_timeout_usec)
            { command_timeout_usec=_command_timeout_usec; }

        long long get_hello_timeout_usec(void)
            { return hello_timeout_usec; }

        void set_hello_timeout_usec(long long _hello_timeout_usec)
            { hello_timeout_usec=_hello_timeout_usec; }

        virtual void set_logging(std::ostream *_logstream, log_level_values _log_level);

        std::ostream* get_logstream(log_level_values level);
};

// contains the internal stuff needed for T2nWrapper
namespace detail
{

    template< typename T >
    struct TypeWrap
    {
        typedef T type;
    };

    template< >
    struct TypeWrap< void >
    {
        typedef int type;
    };

    template< typename R >
    struct Call
    {
        typedef boost::function< R() > FuncType;

        FuncType function;
        R& result;

        Call( FuncType f, R& res ) : function(f), result( res ) {}

        void operator()()
        {
            result= function();
        }
    };

    template< >
    struct Call<void>
    {
        typedef boost::function< void() > FuncType;
        typedef TypeWrap< void >::type ResultType;

        FuncType function;
        ResultType& result;

        Call( FuncType f, ResultType& res ) : function(f), result( res ) {}

        void operator()()
        {
            function();
            result= ResultType();
        }
    };
} // eo namespace detail

class T2nSingletonWrapperMessages
{
    protected:
        static const char* NotInitializedMessage;
};

/** @brief wrap calls to server-side-functions with different error-handling strategies

    Template class to access a process-wide singleton server-connection and to wrap all
    calls using this connection with an error-handling strategy (e.g. to reconnect when
    the connection broke). The source looks very complicated due to heavy use of templates,
    look at the 3rd codegen example to see how to use it.

    @par Example
    Calling remote methods is usually done via t2n_exec, this saves you from always
    specifying which T2nSingletonWrapper-template to use when calling T2nSingletonWrapper::exec
    @code
    t2n_exec(&cmd_group_t2nexample_client::testfunc)("the answer is %d",42)
    @endcode
*/
template< class Client >
class T2nSingletonWrapper : public T2nSingletonWrapperMessages
{
    private:
        std::auto_ptr<Client> Stub;

        static std::auto_ptr<T2nSingletonWrapper> SingletonObject;
        static std::auto_ptr<ConnectionWrapper> WrappedConnection;

        /// @cond
        // create a prep-method for each possible number of parameters
#define _GEN_ARG(z,n,d) Arg ## n arg ##n
#define _GEN_PREP(z,n,d) \
        template< typename R  BOOST_PP_COMMA_IF(n) BOOST_PP_ENUM_PARAMS(n,typename Arg) > \
        static boost::function< R(Client*) > prep \
        ( \
            R(Client::*f)( BOOST_PP_ENUM_PARAMS(n,Arg) ) \
            BOOST_PP_COMMA_IF(n) \
            BOOST_PP_ENUM( n, _GEN_ARG, ~ ) \
        ) \
        { \
            return boost::bind< R > \
                ( \
                    f, _1  BOOST_PP_COMMA_IF(n) BOOST_PP_ENUM_PARAMS(n,arg) \
                ); \
        } // eo prep

        BOOST_PP_REPEAT( BOOST_PP_ADD(T2N_SINGLETON_WRAPPER_MAX_ARGS,1) , _GEN_PREP, ~ )

#undef _GEN_PREP
#undef _GEN_ARG
        /// @endcond

        T2nSingletonWrapper(std::auto_ptr<Client> stub)
        {
            Stub=stub;
        }

        static void init()
        {
            if (WrappedConnection.get() == NULL)
                throw std::logic_error(NotInitializedMessage);

            std::auto_ptr<Client> stub(new Client(WrappedConnection->get_connection(),
                WrappedConnection->get_command_timeout_usec(),
                WrappedConnection->get_hello_timeout_usec()));

            SingletonObject=std::auto_ptr<T2nSingletonWrapper>(new T2nSingletonWrapper(stub));
        }

        template< typename R >
        static
        typename detail::TypeWrap<R>::type real_exec( boost::function< R(Client*) > f)
        {
            ensure_singleton_there();

            typename detail::TypeWrap<R>::type result;

            // bind our Client-object and the local result
            detail::Call<R> call( boost::bind( f, SingletonObject->Stub.get()), result );

            // let the wrapper-handler call the fully-bound function
            if (!WrappedConnection->handle(SingletonObject->Stub.get(),call))
            {
                // create an result with default-constructor if the handler could not
                // successfully do a call but no exception occured
                result=typename detail::TypeWrap<R>::type();
            }
            return result;
        }

    public:

        /** @brief tell the wrapper which connection to use
            @param wrappedConnection the connection to establish when needed
        */
        static void set_connection(std::auto_ptr<ConnectionWrapper> wrappedConnection)
        {
            WrappedConnection=wrappedConnection;

            // reset the singleton to NULL because the singleton must be constructed with current WrappedConnection
            if (SingletonObject.get() != NULL)
                SingletonObject.reset();
        }

        /// return a pointer to the ConnectionWrapper currently in use
        static ConnectionWrapper* get_connection_wrapper(void)
            { return WrappedConnection.get(); }

        /// manually establish the connection without actually executing a call
        static void ensure_singleton_there(void)
        {
            if (SingletonObject.get() == NULL)
                init();
        }

        /// @cond
        // create an exec-method for each possible number of parameters
#define _GEN_PLACEHOLDER(z,n,d) BOOST_PP_CAT(_,BOOST_PP_ADD(n,1))
#define _GEN_EXEC(z,n,d) \
        template< typename R  BOOST_PP_COMMA_IF(n) BOOST_PP_ENUM_PARAMS(n,typename Arg) > \
        static boost::function< R( BOOST_PP_ENUM_PARAMS(n,Arg) ) > exec \
        ( \
            R(Client::*f)( BOOST_PP_ENUM_PARAMS(n,Arg) ) \
        ) \
        { \
            boost::function<R(Client*)>(*p)(R(Client::*)( BOOST_PP_ENUM_PARAMS(n,Arg) ) \
                    BOOST_PP_COMMA_IF(n) BOOST_PP_ENUM_PARAMS(n,Arg)) \
                = &T2nSingletonWrapper::template prep<R BOOST_PP_COMMA_IF(n) BOOST_PP_ENUM_PARAMS(n,Arg) >; \
            return boost::bind \
            ( \
                T2nSingletonWrapper::template real_exec<R>, \
                boost::bind( p, f BOOST_PP_COMMA_IF(n) \
                BOOST_PP_ENUM(n, _GEN_PLACEHOLDER, ~ ) ) \
            ); \
        } // eo exec

        BOOST_PP_REPEAT( BOOST_PP_ADD(T2N_SINGLETON_WRAPPER_MAX_ARGS,1), _GEN_EXEC, ~ )

#undef _GEN_EXEC
#undef _GEN_PLACEHOLDER
        /// @endcond

};

/// @cond
// create an t2n_exec-method for each possible number of parameters
#define _GEN_EXEC(z,n,d) \
        template< class Client, typename R  BOOST_PP_COMMA_IF(n) BOOST_PP_ENUM_PARAMS(n,typename Arg) > \
        static boost::function< R( BOOST_PP_ENUM_PARAMS(n,Arg) ) > t2n_exec \
        ( \
            R(Client::*f)( BOOST_PP_ENUM_PARAMS(n,Arg) ) \
        ) \
        { \
            return T2nSingletonWrapper<Client>::exec(f); \
        } // eo exec

        BOOST_PP_REPEAT( BOOST_PP_ADD(T2N_SINGLETON_WRAPPER_MAX_ARGS,1), _GEN_EXEC, ~ )

#undef _GEN_EXEC
#undef _GEN_PLACEHOLDER
/// @endcond

}
#endif
