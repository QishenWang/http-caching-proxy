#include <pthread.h>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <string>

#include <vector>
#include <time.h>
#include <fstream>
#include <sstream>
#include <ctime>
#include <utility>
#include <algorithm>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>

#include "ResponseParser.h"
#include "Helper.h"
//#include "ThreadInfo.h"
#include "ThreadObj.h"

using namespace std;

class Proxy {
    private:
        Cache* cache = new Cache();
    public:
        ~Proxy();
        void run(const char *port);
        int buildServerAndListen(const char *port);
        int connectToServer(const char *hostname, const char *port);
        int acceptClient(int socket_fd);

        static void* handleGET(int client_connection_fd, vector<char> buffer, RequestParser qp, int num_bytes, Cache* cache, int clientId, int server_fd);
        static void* handlePOST(int client_connection_fd, vector<char> buffer, RequestParser qp, int num_bytes, int clientId, int server_fd);
        static void * handleCONNECT(int client_connection_fd, RequestParser qp, int clientId, int server_fd);
        static void* route(void * info);

        static void checkCorrupt(int clientId, int client_connection_fd);
        static string checkNullStr(string s);
        static bool isChunked(char * serverResp, int numBytes);
        static int tryGetContentLength(string serverResp, int mes_len);
        static void getStringIntoTime(string time);
        static time_t getCurrentTime();
        static time_t getTimeFromString(string time);
        static string getStringFromTime(time_t rawtime);
        static int comparetime(time_t time1,time_t time2);
        static bool isExpire(time_t toCheck);
        static void revalidate(CacheInfo* ci, RequestParser &qp, int client_connection_fd, vector<char> &buffer, int num_bytes, Cache* cache, int server_fd, int clientId);
        static void sendToServer(RequestParser &qp, int client_connection_fd, vector<char> &buffer,int num_bytes, Cache* cache, int server_fd, int clientId);
        static void becomeDaemon();
};