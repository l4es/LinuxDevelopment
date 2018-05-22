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
/** 
 * @file
 * @brief a pointer list
 *
 * provides a pointer list which is prepared for being used in nested
 * iterator loops.
 *
 * @author Reinhard Pfau \<reinhard.pfau@intra2net.com\>
 *
 * @copyright &copy; Copyright 2008 Intra2Net AG
 */

#ifndef __ASYNCIO__PTR_LIST_HPP__
#define __ASYNCIO__PTR_LIST_HPP__

#include <list>
#include <algorithm>


namespace AsyncIo
{
namespace Utils
{


/**
 * @brief provides a global instance counter.
 *
 * The global instance counter must be initialized by the module which uses it.
 * The main purpose for this is to keep track of the instances when PtrList's are
 * used as global objects.
 */
template< class T, bool globalcounted >
class GlobalCounted
{
        static int InstanceCount;

    public:

        GlobalCounted()
        {
            ++InstanceCount;
        } // eo GlobalCounted


        virtual ~GlobalCounted()
        {
            --InstanceCount;
        } // eo ~GlobalCounted

        static int Instances()
        {
            return InstanceCount;
        }
}; // eo GlobalCounted


template<>
class GlobalCounted< class T, false >
{
    public:
        GlobalCounted()
        {
        } // eo GlobalCounted


        virtual ~GlobalCounted()
        {
        } // eo ~GlobalCounted

}; // eo GlobalCounted



/**
 * @brief pointer list which supports deletion by set to NULL.
 *
 * This list can be used in nested iterator loops as long as some conditions
 * are fulfilled:
 * - inside the loops each iterator value is tested for not get NULL.
 * - cleaning the list is only allowed when the outmost loop exits.
 * .
 * 
 *
 * @tparam T type of the pointers.
 * @tparam globalcounted determine if a global instance counter should be included.
 */
template<class T, bool globalcounted= false>
class PtrList
: public GlobalCounted< PtrList< T, globalcounted >, globalcounted >
, protected std::list<T*>
{
        typedef std::list<T*> inherited;

    public:
        typedef GlobalCounted< PtrList< T, globalcounted >, globalcounted > GlobalCountType;

        typedef typename inherited::iterator iterator;
        typedef typename inherited::const_iterator const_iterator;

    public:

        PtrList()
        : Dirty(false)
        {
        } // eo PtrList


        ~PtrList()
        {
        } // eo ~PtrList


        /**
         * @brief add a new item pointer to the list.
         *
         * @param item item pointer which should be added
         */
        void add_item(T* item)
        {
            typename inherited::iterator it= std::find(inherited::begin(), inherited::end(), item);
            if (it != inherited::end())
            {
                // nothing to do since item is already in the list
                return;
            }
            this->push_back(item);
        } // eo add


        /**
         * @brief remove an item pointer from the list by setting to NULL.
         *
         * This sets the pointer in the list to NULL and marks the list as dirty.
         *
         * @param item the io object which should be removed from the list.
         */
        void remove_item(T* item)
        {
            typename inherited::iterator it= std::find(inherited::begin(), inherited::end(), item);
            if (it == inherited::end())
            {
                // nothing to do:
                return;
            }
            *it = NULL; // forget the pointer
            Dirty= true; // ..and mark the list as dirty (i.e. has NULL elements)
        } // eo remove


        /**
         * @brief cleans the list of objects by removing the NULL elements (if any).
         *
         * @note this function should only be called when it is ensured that no
         * other functions using iterators of this list.
         */
        void clean_list()
        {
            if (!Dirty)
            {
                // nothing to do
                return;
            }
            // remove the NULL elements now:
            this->erase(
                std::remove( inherited::begin(), inherited::end(), (T*)NULL),
                inherited::end() );
            Dirty= false;
        } // eo clean_list


        /**
         * @brief explicit set the list dirty.
         *
         * use this to mark the list as dirty, when an item is set to NULL via
         * a non const iterator.
         *
         * @note This is really dirty; better use remove_item(T*) in that cases!
         */
        void set_dirty()
        {
            Dirty= true;
        } // eo set_dirty


        iterator begin()
        {
            return inherited::begin();
        }


        iterator end()
        {
            return inherited::end();
        }


        const_iterator begin() const
        {
            return inherited::begin();
        }


        const_iterator end() const
        {
            return inherited::end();
        }

    protected:
        bool Dirty;

}; // eo class PtrList


} // eo namespace Utils
} // eo namespace AsyncIo

#endif
