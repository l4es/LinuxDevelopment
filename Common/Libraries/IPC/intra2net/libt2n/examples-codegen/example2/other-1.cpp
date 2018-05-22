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
#include "other_common.hxx"

using namespace std;

//! test function overload
LIBT2N_EXPORT int t3(int i)
{
    return i;
}

//! test pair, multiple arguments and namespace
LIBT2N_EXPORT bool t3(int i, float f, const string &s, const pair<int, float> &p)
{
    return (i==p.first) && (f==p.second) && (s=="hello");
}

//! test std::vector
LIBT2N_EXPORT bool t3(const std::vector<int> &i)
{
    return (i.size()==1) && (i[0]==10);
}
