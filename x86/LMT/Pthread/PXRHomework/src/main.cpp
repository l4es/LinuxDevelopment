#include <iostream>
#include "customthread.h"
#include "server.h"

using namespace std;

int main() {
  cout << "Running!" << endl;

  Server *s;
  s = new Server();

  //Main loop
  s->acceptAndDispatch();

  return 0;
}