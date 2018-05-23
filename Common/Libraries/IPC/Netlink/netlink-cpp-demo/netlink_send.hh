#ifndef __NETLINK_SEND_HH__
#define __NETLINK_SEND_HH__


class NetlinkSend
{
public:
  NetlinkSend();
  ~NetlinkSend();
  
  int
  send(int sock, int type);
};


#endif //__NETLINK_SEND_HH__
