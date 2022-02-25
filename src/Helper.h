#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <string>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace std;

class Helper {
    public:
        static int tryBuildServerAndListen(const char *port);

        static int tryAcceptClient(int socket_fd, string &s);

        static int tryConnectToServer(const char *hostname, const char *port);

        static void* get_in_addr(struct sockaddr *sa);

};
