#include "Proxy.h"
#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <exception>

using namespace std;

int main(int argc, char *argv[]) {
  // Thu, 10 Feb 2022 17:22:33 GMT
  // cout << "--------------------------------------------------------------------------------" << endl;
  // cout << "Testing time handling..." << endl;
  // int test = Proxy::getTimeFromString("Thu, 10 Feb 2022 17:22:33 GMT");
  // cout << "Testing before: " << "Thu, 10 Feb 2022 17:22:33 GMT" << endl;
  // cout << "Testing get time from string: " << test << endl;
  // cout << "Testing get string from response time: " << Proxy::getStringFromTime(test);
  // cout << "Testing get current time: " << Proxy::getCurrentTime() << endl;
  // cout << "Testing get string from current time: " << Proxy::getStringFromTime(Proxy::getCurrentTime());
  // cout << "--------------------------------------------------------------------------------" << endl;
  try{
    // Proxy::becomeDaemon();
    Proxy *proxy = new Proxy();
    proxy->run("12345");
  }
  catch(exception& e){
    cout << e.what();
    return EXIT_FAILURE;
  }
  
  return 0;
}