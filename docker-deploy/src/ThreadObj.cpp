#include "ThreadObj.h"

ThreadObj::ThreadObj(int client_connection_fd, int clientId, Cache* cache, string ip) {
    this->client_connection_fd = client_connection_fd;
    this->clientId = clientId;
    this->cache = cache;
    this->ip = ip;
}
int ThreadObj::getClientConnectionfd() {
    return this->client_connection_fd;
}
int ThreadObj::getClientId() {
    return this->clientId;
}
Cache* ThreadObj::getCache() {
    return this->cache;
}
string ThreadObj::getIp() {
    return this->ip;
}