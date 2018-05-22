/*
    Copyright (C) 2006                                                    
    intra2net.com                                                         

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <socket_client.hxx>

// include library header
#include "minimalistic-client-stub.hxx"

int main(int argc, char** argv)
{
  extended_type_info_test();

  libt2n::socket_client_connection sc("./socket");
  cmd_group_example_client cc(&sc);

  return (cc.testfunc("hello")=="hello, testfunc() was here") ? EXIT_SUCCESS : EXIT_FAILURE;
}
