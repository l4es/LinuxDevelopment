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
 * @brief some functions to wrap some system functions
 *
 * @author Reinhard Pfau \<Reinhard.Pfau@gmx.de\>
 *
 * @copyright &copy; Copyright 2009 by Intra2net AG
 * @contact Intra2net Opensource Team \<opensource@intra2net.com\>
 */


#ifndef _ASYNCIO_SYSTEM_TOOLS_HPP_
#define _ASYNCIO_SYSTEM_TOOLS_HPP_

#include <string>
#include <sys/types.h>
#include <stdint.h>


namespace AsyncIo
{
namespace Utils
{

/**
 * @brief removed a file entry from file system (if existing).
 * @param path path to be removed.
 * @return @a true if the file was successfully deleted.
 */
bool unlink( const std::string& path );


/**
 * @brief represents a file(/path) status.
 *
 * This is basically a wrapper around the "stat" call.
 */
class FileStat
{
    public:
        FileStat( const std::string& path, bool follow_symlinks= true );
        ~FileStat();

        /**
         * @brief stat the path again and updates the values.
         */
        void refresh();

        /**
         * @brief returns if the stat values are valid.
         * @return @a true if the stat values are valid.
         */
        bool is_valid() const { return m_is_valid; }

        operator bool() const { return m_is_valid; }

        mode_t mode() const { return m_mode; }

        bool is_regular_file() const { return m_is_regular_file; }
        bool is_directory() const { return m_is_directory; }
        bool is_character_device() const { return m_is_character_device; }
        bool is_block_device() const { return m_is_block_device; }
        bool is_fifo() const { return m_is_fifo; }
        bool is_symbolic_link() const { return m_is_symbolic_link; }
        bool is_link() const { return m_is_symbolic_link; }
        bool is_socket() const { return m_is_socket; }
        bool is_exec_file() const { return m_is_exec_file; }

    private:
        std::string m_path;
        bool m_follow_symlinks;

        bool m_is_valid;

        mode_t m_mode;

        bool m_is_regular_file      :1;
        bool m_is_directory         :1;
        bool m_is_character_device  :1;
        bool m_is_block_device      :1;
        bool m_is_fifo              :1;
        bool m_is_symbolic_link     :1;
        bool m_is_socket            :1;
        bool m_is_exec_file         :1;

}; // end of FileStat


/*
 * IPv4 support
 */

class IPv4Address
{
    public:
        IPv4Address();
        IPv4Address( uint32_t _address );

        bool is_valid() const {return m_valid;}

        uint32_t get_address() const { return m_address; }

        uint32_t get_address_nbo() const;

    private:

        uint32_t m_address;
        bool m_valid;
}; // end of IPv4Address


} // end of namespace Utils
} // end of namespace AsyncIo


#endif
