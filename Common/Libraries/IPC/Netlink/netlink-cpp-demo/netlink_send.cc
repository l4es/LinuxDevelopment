#include <errno.h>
#include <string.h>
#include <linux/types.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/rtnetlink.h>

#include <vector>
#include <string>
#include <iostream>

#include "netlink_send.hh"

using namespace std;

NetlinkSend::NetlinkSend()
{
}

NetlinkSend::~NetlinkSend()
{
}

int
NetlinkSend::send(int sock, int type)
{
  int ret;
  struct sockaddr_nl snl;

  struct
  {
    struct nlmsghdr nlh;
    struct rtgenmsg g;
  } req;


  /* Check netlink socket. */
  if (sock < 0) {
    cout << "sock is not active, exiting" << endl;
    return -1;
  }

  memset (&snl, 0, sizeof snl);
  snl.nl_family = AF_NETLINK;

  req.nlh.nlmsg_len = sizeof req;
  req.nlh.nlmsg_flags = NLM_F_DUMP | NLM_F_REQUEST;
  req.nlh.nlmsg_pid = getpid();
  req.nlh.nlmsg_type = type;
  req.nlh.nlmsg_seq = time(NULL);
  req.g.rtgen_family = AF_UNSPEC;
 
  ret = sendto (sock, (void*) &req, sizeof req, 0, 
  (struct sockaddr*) &snl, sizeof snl);
  if (ret < 0) {
    cout << "netlink_send failed on send " << endl;
    return -1;
  }
  return 0;
}
