#include <vector>
#include "RequestParser.h"
#include "Cache.h"
#include <string>

class ThreadObj {
    private:
        int client_connection_fd;
        int clientId;
        Cache* cache;
        string ip;
    public:
        ThreadObj(int client_connection_fd, int clientId, Cache* cache, string ip);
        int getClientConnectionfd();
        int getClientId();
        Cache* getCache();
        string getIp();
};