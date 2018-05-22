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

// include generated library headers
#include "default_client.hxx"
#include "other_client.hxx"

template <typename X, typename Y>
std::ostream& operator<<(std::ostream &o, const std::pair<X,Y> &p)
{
  o << "<" << p.first << "," << p.second << ">";
  return o;
}

int main(int argc, char** argv)
{
  libt2n::socket_client_connection sc("./socket");
  libt2n::socket_client_connection sc_other("./socket_other");
  cmd_group_default_client cc(&sc);
  cmd_group_other_client cc_other(&sc_other);

  bool throwok=false;
  try
  {
      cc.testfunc("throw");
  }catch(libt2n::t2n_runtime_error &e){
      throwok=(std::string(e.what())=="throw me around");
  }

  if (throwok) {
    throwok=false;
    try
    {
      cc.t6();
    }catch(libt2n::t2n_runtime_error &e){
      throwok=(std::string(e.what())=="throw me around2");
    }
  }

  Foo foo={10,10};

  std::vector<unsigned> v;
  std::list<unsigned> l;
  std::set<unsigned> s;
  for (unsigned i=1;i<=100;++i) {
    v.push_back(i);
    l.push_back(i);
    s.insert(i);
  }
  unsigned expected_sum=(101*100)/2;

  int exit_status=EXIT_SUCCESS;

#define COMPARE(x,y) do{if (x!=y) {std::cerr << #x << "!=" << #y << ": " << x << "!=" << y << std::endl; exit_status=EXIT_FAILURE;}}while(0)

  COMPARE(throwok, true);
  COMPARE( cc.testfunc("hello"), "hello, testfunc() was here" );
  COMPARE( cc.testfunc_ref("hello"), "hello, testfunc() was here" );
  COMPARE( cc.t2(10), 10 );
  COMPARE( cc.t3(10), 10 );
  COMPARE( cc.t4(static_cast<const int *>(&foo.i)), 10 );
  COMPARE( cc.t5(), 10 );
  COMPARE( cc.sum(v), expected_sum );
  COMPARE( cc.sum(l), expected_sum );
  COMPARE( cc.sum(s), expected_sum );
  COMPARE( cc.pdup(10), (std::pair<int,int>(10,10)));
  // todo: boost doesn't provide serialization for boost tuple :-(
  //  COMPARE( cc.quad(10), boost::tuple<int,int,int,int>(10,10,10,10));
  COMPARE( cc_other.t3(10, 20, "hello", std::pair<int, float>(10,20)), true );
  COMPARE( cc_other.t3(10), 10 );
  COMPARE( cc_other.t3(std::vector<int>(1,10)), true );
  COMPARE( cc_other.t3(foo), true );
  return exit_status;
}
