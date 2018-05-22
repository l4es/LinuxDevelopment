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
#include "default_common.hxx"

//! test pass by copy
LIBT2N_EXPORT std::string testfunc(std::string str) 
{
    std::string ret;
    if (str=="throw")
        throw libt2n::t2n_runtime_error("throw me around");
    if (str=="big")
        ret.insert(0,100*1024,'x');
    else
        ret=str+", testfunc() was here";
    return ret;
}

//! test pass by const reference
LIBT2N_EXPORT std::string testfunc_ref(const std::string &str) 
{
    std::string ret;
    if (str=="throw")
        throw libt2n::t2n_runtime_error("throw me around");
    if (str=="big")
        ret.insert(0,100*1024,'x');
    else
        ret=str+", testfunc() was here";
    return ret;
}

//! test builtin type
LIBT2N_EXPORT int t2(int i) 
{
    return i;
}

//! test const ref of builtin
LIBT2N_EXPORT int t3(const int &i) 
{
    return i;
}

//! test pointer to const data
LIBT2N_EXPORT int t4(const int* i) 
{
    return *i;
}

//! test no args
LIBT2N_EXPORT int t5() 
{
    return 10;
}

//! test no args and no return
LIBT2N_EXPORT void t6() 
{
    throw libt2n::t2n_runtime_error("throw me around2");
}

template <typename X>
unsigned
sumT(const X &x) {
  unsigned res=0;
  for (typename X::const_iterator it=x.begin();it!=x.end();++it)
    res+=*it;
  return res;
}

// todo: export of explicitely instantiated template functions doesn't work

//! test vector
LIBT2N_EXPORT unsigned sum(const std::vector<unsigned> &v)
{return sumT(v);}

//! test list
LIBT2N_EXPORT unsigned sum(const std::list<unsigned> &v)
{return sumT(v);}

//! test set
LIBT2N_EXPORT unsigned sum(const std::set<unsigned> &v)
{return sumT(v);}

//! test return of std::pair
LIBT2N_EXPORT std::pair<int,int> pdup(int i) {
  return std::pair<int,int>(i,i);
}

/*
  todo: boost doesn't provide serialization for boost tuple :-(
  //! test return of boost::tuple
  LIBT2N_EXPORT boost::tuple<int,int,int,int> quad(int i) {
  return std::tuple<int,int,int,int>(i,i,i,i);
}
*/
