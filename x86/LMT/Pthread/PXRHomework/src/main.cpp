#include <iostream>
#include "customthread.h"
#include "server.h"

using namespace std;

int main(int argc, char** argv) {

  if (argc < 3) {
    printf("usage: %s <port> <ip>\n", argv[0]);
    exit(1);
  }

  cout << "Running!" << endl;

  Server *s = new Server(atoi(argv[1]), argv[2]);

  //Main loop
  s->dispatchConnection();

  // Free memory
  delete s;

  return 0;
}