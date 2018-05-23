#ifndef __NETLINK_TYPES_HH__
#define __NETLINK_TYPES_HH__


#include <stdint.h>
#include <sys/types.h>
#include <net/ethernet.h>
#include <netinet/ether.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


class IPv4
{
public:
  IPv4() : _addr(0) {}
  IPv4(uint32_t addr) : _addr(addr) {}

  std::string
  str() {
    struct in_addr a;
    a.s_addr = _addr;
    return std::string(inet_ntoa(a));
  }
  
private:
  uint32_t _addr;
};

class Mac
{
public:
  Mac() {}
  Mac(struct ether_addr mac) {
    _mac = mac;
  }

private:
  struct ether_addr _mac;

};


#endif //__NETLINK_TYPES_HH__
