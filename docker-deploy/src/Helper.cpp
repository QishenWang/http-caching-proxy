#include "Helper.h"

using namespace std;

int Helper::tryBuildServerAndListen(const char *port) {

    int status;
    int socket_fd;
    struct addrinfo host_info;
    struct addrinfo *host_info_list;
    const char *hostname = NULL;
    //const char *port     = port;

    memset(&host_info, 0, sizeof(host_info));

    host_info.ai_family   = AF_UNSPEC;
    host_info.ai_socktype = SOCK_STREAM;
    host_info.ai_flags    = AI_PASSIVE;

    status = getaddrinfo(hostname, port, &host_info, &host_info_list);
    if (status != 0) {
        // cerr << "Error: cannot get address info for host" << endl;
        // cerr << "  (" << hostname << "," << port << ")" << endl;
        throw "Error: cannot get address info for host";
    } //if

    socket_fd = socket(host_info_list->ai_family, 
                host_info_list->ai_socktype, 
                host_info_list->ai_protocol);
    if (socket_fd == -1) {
        // cerr << "Error: cannot create socket" << endl;
        // cerr << "  (" << hostname << "," << port << ")" << endl;
        throw "Error: cannot create socket";
    } //if

    int yes = 1;
    status = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    status = bind(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
    if (status == -1) {
        // cerr << "Error: cannot bind socket" << endl;
        // cerr << "  (" << hostname << "," << port << ")" << endl;
        throw "Error: cannot bind socket";
    } //if

    status = listen(socket_fd, 100);
    if (status == -1) {
        // cerr << "Error: cannot listen on socket" << endl; 
        // cerr << "  (" << hostname << "," << port << ")" << endl;
        throw "Error: cannot listen on socket";
    } //if

    freeaddrinfo(host_info_list);
    // cout << "Waiting for connection on port " << port << endl;
    return socket_fd;
}

int Helper::tryAcceptClient(int socket_fd, string &s) {

    char ip[INET6_ADDRSTRLEN];
    struct sockaddr_storage socket_addr;
    socklen_t socket_addr_len = sizeof(socket_addr);
    int client_connection_fd;
    client_connection_fd = accept(socket_fd, (struct sockaddr *)&socket_addr, &socket_addr_len);
    if (client_connection_fd == -1) {
        // cerr << "Error: cannot accept connection on socket" << endl;
        throw "Error: cannot accept connection on socket";
    } //if
    inet_ntop(socket_addr.ss_family, get_in_addr((struct sockaddr *)&socket_addr), ip, sizeof ip);
    string tmp(ip);
    s = tmp;
    return client_connection_fd;
}
void *Helper::get_in_addr(struct sockaddr *sa)
{
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }
  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
int Helper::tryConnectToServer(const char *hostname, const char *port) {
    int status;
    int socket_fd;
    struct addrinfo host_info;
    struct addrinfo *host_info_list;

    memset(&host_info, 0, sizeof(host_info));
    host_info.ai_family   = AF_UNSPEC;
    host_info.ai_socktype = SOCK_STREAM;

    status = getaddrinfo(hostname, port, &host_info, &host_info_list);
    if (status != 0) {
        // cerr << "Error: cannot get address info for host" << endl;
        // cerr << "  (" << hostname << "," << port << ")" << endl;
        throw "Error: cannot get address info for host";
    } //if

    socket_fd = socket(host_info_list->ai_family, 
                host_info_list->ai_socktype, 
                host_info_list->ai_protocol);
    if (socket_fd == -1) {
        // cerr << "Error: cannot create socket" << endl;
        // cerr << "  (" << hostname << "," << port << ")" << endl;
        throw "Error: cannot create socket";
    } //if
    
    // cout << "Connecting to " << hostname << " on port " << port << "..." << endl;
    
    status = connect(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
    if (status == -1) {
        // cerr << "Error: cannot connect to socket" << endl;
        // cerr << "  (" << hostname << "," << port << ")" << endl;
        // return -1;
        throw "Error: cannot connect to socket";
    } //if

    freeaddrinfo(host_info_list);
    return socket_fd;
}