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
 * @brief some helper for handling (STL) data structures.
 *
 * @author Reinhard Pfau \<Reinhard.Pfau@intra2net.com\>
 *
 * (c) Copyright 2007-2008 by Intra2net AG
 * 
 * opensource@intra2net.com
 */

#ifndef _I2N_CONTAINERFUNC_HPP_
#define _I2N_CONTAINERFUNC_HPP_

#include <map>
#include <list>
#include <set>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>


namespace I2n {


/**
 * convenience class to insert key-value pairs into a map.
 */
template<typename K, typename V>
class MapFiller
{
        std::map<K,V> & MapRef;

    public:
        MapFiller( std::map<K,V> & map_ref)
        : MapRef(map_ref)
        {
        }

        MapFiller& operator () (const K& key, const V& value)
        {
            MapRef[key]= value;
            return *this;
        }

}; // eo class MapFiller


/**
 * convenience class to fill values into a container (using push_back).
 */
template<
    typename T,
    template< typename, typename> class C= std::list,
    typename Alloc = std::allocator< T >
>
class PushBackFiller
{
        C< T, Alloc > &CRef;
    public:
        PushBackFiller( C<T, Alloc > & c )
        : CRef(c)
        {}

        PushBackFiller& operator () (const T& value)
        {
            CRef.push_back(value);
            return *this;
        } // eo operator ()

}; // eo class PushBackFiller


template<
    typename T,
    template< typename, typename > class C,
    typename Alloc
>
PushBackFiller< T, C, Alloc > get_push_back_filler( C< T, Alloc >& c)
{
    return PushBackFiller< T, C, Alloc >(c);
} // eo get_push_back_filler(C< T, Alloc >&)


/**
 * convenience class for transient construction of a container including values.
 */
template<
    typename T,
    template< typename, typename> class C= std::list,
    typename Alloc = std::allocator< T >
>
class TransientPushBackFiller
{
        C< T, Alloc > MyC;
    public:
        typedef C< T, Alloc > CType;
    
        TransientPushBackFiller()
        {}
        
        TransientPushBackFiller& operator () (const T& value)
        {
            MyC.push_back(value);
            return *this;
        } // eo operator ()
        
        operator CType () const { return MyC; }
}; // eo class TransientPushBackFiller


/**
 * convenience class for transient construction of a map including values.
 */
template<typename K, typename V>
class TransientMapFiller
{
        std::map<K,V> Map;
    
    public:
        typedef std::map< K, V > CType;
    
    
        TransientMapFiller( )
        {
        }
        
        TransientMapFiller& operator () (const K& key, const V& value)
        {
            Map[key]= value;
            return *this;
        }
        
        operator CType () const { return Map; }
}; // eo class MapFiller




/**
 * returns the keys of a map as a list.
 * @param the_map the map.
 * @param the_keys the list where the keys are added to.
 * @return @a true.
 */
template< typename K, typename V >
bool get_key_list( const std::map< K, V >& the_map, std::list< K >& the_keys )
{
    for(typename std::map< K, V >::const_iterator it= the_map.begin();
        it != the_map.end();
        ++it )
    {
        the_keys.push_back( it->first );
    }
    return true;
}  // eo get_key_list(const std::map< K,V >, std::list< K >&)


/**
 * returns the keys of a map as a list.
 * @param the_map the map.
 * @return the list of keys.
 */
template< typename K, typename V >
std::list< K > get_key_list( const std::map< K, V>& the_map)
{
    std::list< K > result;
    get_key_list(the_map, result);
    return result;
} // eo get_key_list(const std::map< K,V >)


/**
 * returns the keys of a map as a set.
 * @param the_map the map.
 * @param the_keys the set where the keys are added to.
 * @return @a true.
 */
template< typename K, typename V >
bool get_key_set( const std::map< K, V >& the_map, std::set< K >& the_keys )
{
    for(typename std::map< K, V >::const_iterator it= the_map.begin();
        it != the_map.end();
        ++it )
    {
        the_keys.insert( it->first );
    }
    return true;
}  // eo get_key_set(const std::map< K,V >, std::set< K >&)


/**
 * returns the keys of a map as a set.
 * @param the_map the map.
 * @return the set of keys.
 */
template< typename K, typename V >
std::list< K > get_key_set( const std::map< K, V>& the_map)
{
    std::set< K > result;
    get_key_set(the_map, result);
    return result;
} // eo get_key_set(const std::map< K,V >)


/**
 * functor version of new().
 * can be used for deferred instantiation of objects.
 */
template<
    typename T
>
struct New
{
    typedef T* result_type;
    
    result_type operator() () 
    {
        return new T;
    }
    
    template<typename Arg1>
    result_type operator() (Arg1 arg1)
    {
        return new T(arg1);
    }
    
    template<typename Arg1, typename Arg2>
    result_type operator() (Arg1 arg1,Arg2 arg2)
    {
        return new T(arg1,arg2);
    }
    
    template<typename Arg1, typename Arg2, typename Arg3>
    result_type operator() (Arg1 arg1,Arg2 arg2,Arg3 arg3)
    {
        return new T(arg1,arg2,arg3);
    }
    
    template<typename Arg1, typename Arg2, typename Arg3, typename Arg4>
    result_type operator() (Arg1 arg1,Arg2 arg2,Arg3 arg3,Arg4 arg4)
    {
        return new T(arg1,arg2,arg3,arg4);
    }
    
    template<typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5>
    result_type operator() (Arg1 arg1,Arg2 arg2,Arg3 arg3,Arg4 arg4,Arg5 arg5)
    {
        return new T(arg1,arg2,arg3,arg4,arg5);
    }
    
}; // eo struct New




/**
 * functor version of boost::shared_ptr = new().
 * can be used for deferred instantiation of objects.
 */
template<
    typename T
>
struct SharedPtrNew
{
    typedef boost::shared_ptr<T> result_type;
    
    result_type operator() () 
    {
        return result_type(new T);
    }
    
    template<typename Arg1>
    result_type operator() (Arg1 arg1)
    {
        return result_type(new T(arg1));
    }
    
    template<typename Arg1, typename Arg2>
    result_type operator() (Arg1 arg1,Arg2 arg2)
    {
        return result_type(new T(arg1,arg2));
    }
    
    template<typename Arg1, typename Arg2, typename Arg3>
    result_type operator() (Arg1 arg1,Arg2 arg2,Arg3 arg3)
    {
        return result_type(new T(arg1,arg2,arg3));
    }
    
    template<typename Arg1, typename Arg2, typename Arg3, typename Arg4>
    result_type operator() (Arg1 arg1,Arg2 arg2,Arg3 arg3,Arg4 arg4)
    {
        return result_type(new T(arg1,arg2,arg3,arg4));
    }
    
    template<typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5>
    result_type operator() (Arg1 arg1,Arg2 arg2,Arg3 arg3,Arg4 arg4,Arg5 arg5)
    {
        return result_type(new T(arg1,arg2,arg3,arg4,arg5));
    }
    
}; // eo struct SharedPtrNew



/*
** classes holding a shared or weak pointer.
** designed to be used as policy classes...
** (but may be used for other purposes, too)
*/

template<
    class X
>
class SharedOwnership
{
    public:
    
        SharedOwnership( boost::shared_ptr< X > ptr)
        : StoredPtr(ptr)
        {}
        
        
        boost::shared_ptr< X > get_ptr() const { return StoredPtr; }
    
    public:
    
        boost::shared_ptr< X > StoredPtr;
}; // eo class SharedOwnership



template<
    class X
>
class WeakOwnership
{
    public:
    
        WeakOwnership( boost::shared_ptr< X > ptr)
        : StoredPtr(ptr)
        {}
        
        
        boost::shared_ptr< X > get_ptr() const { return StoredPtr.lock(); }
    
    public:
    
        boost::weak_ptr< X > StoredPtr;
}; // eo class WeakOwnership



/*
** NoOperation
** ===========
*/


/**
 * @brief the ultimate class for doing nothing :-)
 *
 * Even it sounds odd to have a functor class which is doing nothing it makes
 * sense to use this class in some situations.
 *
 * One example is the usage as Deleter for shared pointers which "own" a pointer to
 * a static object (i.e. it is not allowed to be really deleted...). Using this class
 * as Deleter in these pointers enables us to use interfaces which expect a shared pointer
 * even if we want(/must) to pass pointers to static objects.
 */
struct NoOperation
{
    NoOperation() {};

    template<
        typename Arg1
    >
    NoOperation(Arg1) {}

    template<
        typename Arg1, typename Arg2
    >
    NoOperation(Arg1, Arg2) {}

    template<
        typename Arg1, typename Arg2, typename Arg3
    >
    NoOperation(Arg1, Arg2, Arg3) {}

    void operator() () const {}

    template<
        typename Arg1
    >
    void operator() (Arg1) const {}

    template<
        typename Arg1, typename Arg2
    >
    void operator() (Arg1, Arg2) const {}

    template<
        typename Arg1, typename Arg2, typename Arg3
    >
    void operator() (Arg1, Arg2, Arg3) const {}

}; // eo struct NoOperation



} // eo namespace I2n

#endif
