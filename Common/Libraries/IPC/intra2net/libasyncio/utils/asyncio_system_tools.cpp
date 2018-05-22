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
 * @copyright &copy; Copyright 2009 by Intra2net AG
 */

#include "asyncio_system_tools.hpp"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <netinet/in.h>


namespace AsyncIo
{
namespace Utils
{

/*
 * implementation of FileStat
 */

FileStat::FileStat( const std::string& path, bool follow_symlinks)
: m_path( path )
, m_is_valid( false )
, m_follow_symlinks( follow_symlinks )
{
    refresh();
}// end of FileStat::FileStat()


FileStat::~FileStat()
{
} // end of FileStat::~FileStat()


void FileStat::refresh()
{
    struct stat file_stat[1];

    int res;

    res= (m_follow_symlinks ? ::stat : ::lstat)( m_path.c_str(), file_stat );

    if (res)
    {
        m_is_valid= false;
        return;
    }

    m_mode= file_stat->st_mode;

    m_is_regular_file= S_ISREG(file_stat->st_mode);
    m_is_directory= S_ISDIR(file_stat->st_mode);
    m_is_character_device= S_ISCHR(file_stat->st_mode);
    m_is_block_device= S_ISBLK(file_stat->st_mode);
    m_is_fifo= S_ISFIFO(file_stat->st_mode);
    m_is_symbolic_link= S_ISLNK(file_stat->st_mode);
    m_is_socket= S_ISSOCK(file_stat->st_mode);
    m_is_exec_file = m_is_regular_file && (file_stat->st_mode & (S_IXUSR | S_IXGRP | S_IXOTH));
    // TODO: implement more!
} // end of FileStat::refresh()



/*
 * funcs
 */

bool unlink( const std::string& path )
{
    return ::unlink( path.c_str() ) == 0;
} // end of unlink(const std::string&)



/*
 * IPv4
 */

IPv4Address::IPv4Address( )
: m_address( 0u )
, m_valid( false )
{
}


IPv4Address::IPv4Address(uint32_t _address)
: m_address( _address )
, m_valid( true )
{
}


uint32_t IPv4Address::get_address_nbo() const
{
    return htonl( m_address );
}



} // end of namespace Utils
}// end of namespace AsyncIo
