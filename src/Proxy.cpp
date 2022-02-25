#include "Proxy.h"

#include <vector>

using namespace std;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static ofstream log("/var/log/erss/proxy.log");

Proxy::~Proxy() {
    delete(this->cache);
} 
void Proxy::run(const char *port) {
    int socket_fd = Helper::tryBuildServerAndListen(port);
    int clientId = 0;
    while (1) {
        string ip = "";
        int client_connection_fd = Helper::tryAcceptClient(socket_fd, ip);
        //pthread_t thread;
        pthread_mutex_lock(&mutex);
        ThreadObj* threadObj = new ThreadObj(client_connection_fd, clientId++, this->cache, ip);
        pthread_mutex_unlock(&mutex);
        thread threadTmp (Proxy::route, threadObj);
        //pthread_create(&thread, NULL, Proxy::route, threadObj);
        threadTmp.detach();
        //pthread_detach(thread);
    }
}
void* Proxy::route(void * info) {
    int client_connection_fd = -1;
    int server_fd = -1;
    try {
        ThreadObj * threadObj = (ThreadObj *) info;
        client_connection_fd = threadObj->getClientConnectionfd();
        int clientId = threadObj->getClientId();
        Cache * cache = threadObj->getCache();
        string ip = threadObj->getIp();

        delete(threadObj);
        
        vector<char> buffer (1024 * 1024);
        int num_bytes = recv(client_connection_fd, &buffer.data()[0], 1000 * 1000, 0);
        if (num_bytes <= 0) {
            checkCorrupt(clientId, client_connection_fd);
            return NULL;
        }
        buffer.data()[num_bytes] = '\0';

        RequestParser qp;
        qp.parse(buffer);
        string method = qp.getMethod();

        string receivedTime = Proxy::getStringFromTime(Proxy::getCurrentTime());
        pthread_mutex_lock(&mutex);
        log << clientId << ": \"" << qp.getMethodLine() << "\" from " << ip << " @ " << receivedTime;
        pthread_mutex_unlock(&mutex);

        string s = qp.getHostname();

        if (method == "GET") {
            server_fd = Helper::tryConnectToServer(s.c_str(), "80");
            if (server_fd == -1) {
                terminate();
            }
            Proxy::handleGET(client_connection_fd, buffer, qp, num_bytes, cache, clientId, server_fd);
        }
        else if (method == "POST") {
            server_fd = Helper::tryConnectToServer(s.c_str(), "80");
            if (server_fd == -1) {
                terminate();
            }
            Proxy::handlePOST(client_connection_fd, buffer, qp, num_bytes, clientId, server_fd);
        }
        else if (method == "CONNECT") {
            string s = qp.getHostname();
            int idx = s.find(":");
            string connectHost = s.substr(0, idx);
            string connectPort = s.substr(idx + 1, s.length() - idx - 1);
            int server_fd = Helper::tryConnectToServer(connectHost.c_str(), connectPort.c_str());
            if (server_fd == -1) {
                terminate();
            }
            Proxy::handleCONNECT(client_connection_fd, qp, clientId, server_fd);
        }
        else {
            pthread_mutex_lock(&mutex);
            log << clientId << ": " << "Responding \"HTTP1.1 400 ERROR\"" << endl;
            pthread_mutex_unlock(&mutex);
            send(client_connection_fd, "HTTP/1.1 400 ERROR\r\n\r\n", 22, 0);
        }
        close(server_fd);
        close(client_connection_fd);
        return NULL;
    }
    catch(const char *msg) {
        if (client_connection_fd != -1) {
            close(client_connection_fd);
        }
        if (server_fd != -1) {
            close(server_fd);
        }
        cout << msg << endl;
        return NULL;
    }
}
string Proxy::checkNullStr(string s) {
    if (s.length() == 0) {
        return " ";
    }
    return s;
}
void Proxy::checkCorrupt(int clientId, int client_connection_fd) {
    pthread_mutex_lock(&mutex);
    log << clientId << ": " << "Responding \"HTTP/1.1 502 ERROR\"" << endl;
    pthread_mutex_unlock(&mutex);
    int sendRes = send(client_connection_fd, "HTTP/1.1 502 ERROR\r\n\r\n", 22, 0);
    if (sendRes == -1) {
        throw "Send fails";
    }
}
bool Proxy::isChunked(char * serverResp, int numBytes) {
    string s(serverResp, numBytes);
    size_t idx;
    if ((idx = s.find("chunked")) != string::npos) {
        return true;
    }
    return false;
}
int Proxy::tryGetContentLength(string serverResp, int mes_len) {
    stringstream ss(serverResp);
    string tmp;
    while (getline(ss, tmp)) {
        size_t idx;
        if ((idx = tmp.find("Content-Length: ")) != string::npos) {
            size_t head_end = serverResp.find("\r\n\r\n");
            int part_body_len = mes_len - (int) (head_end) - 8;
            int num = atoi(tmp.substr(idx + 16, tmp.length() - 16).c_str());
            // cout << "mes_len      : " << mes_len << endl;
            // cout << "head_end     : " << head_end << endl;
            // cout << "part_body_len: " << part_body_len << endl;
            // cout << "num          : " << num << endl;
            return num - part_body_len - 4;
        }
    }
    return -1;
}

void Proxy::becomeDaemon(){
    // First fork, allow the child to setsid()
    pid_t pid;

    /* Fork off the parent process */
    pid = fork();

    /* An error occurred */
    if (pid < 0)
        exit(EXIT_FAILURE);

    /* Success: Let the parent terminate */
    if (pid > 0)
        exit(EXIT_SUCCESS);

    /* On success: The child process becomes session leader */
    if (setsid() < 0)
        exit(EXIT_FAILURE);

    // Change working directory to "/"
    if (chdir("/") < 0) {
        throw runtime_error("Cannot change working directory to /");
    }

    // Clear umask
    umask(0);

    // Second fork, make the child not a session leader
    pid_t pid2;

    /* Fork off the parent process */
    pid2 = fork();

    /* An error occurred */
    if (pid2 < 0)
        exit(EXIT_FAILURE);

    /* Success: Let the parent terminate */
    if (pid2 > 0)
        exit(EXIT_SUCCESS);

    /* On success: The child process becomes session leader */
    if (setsid() < 0)
        exit(EXIT_FAILURE);
        
    // Here is the child, try to point stdin/out/err at /dev/null
    int null_fd = open("/dev/null", O_RDWR);
    if (null_fd < 0) {
        throw runtime_error("Cannot open /dev/null");
    }
    if (dup2(null_fd, STDIN_FILENO) < 0 || dup2(null_fd, STDOUT_FILENO < 0) || dup2(null_fd, STDERR_FILENO) < 0) {
        throw runtime_error("Cannot redirect stdin/out/err");
    }
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
}

void* Proxy::handleGET(int client_connection_fd, vector<char> buffer, RequestParser qp, int num_bytes, Cache* cache, int clientId, int server_fd) {
    cout << "Client " << clientId << ": " << "Handling: " << qp.getMethodLine() << endl;
    cout << "---------------------------------------------------------------------------" << endl;
    cout << "<GET LOG BEGINS>" << endl;
    // SEND REQUEST TO SERVER

    pthread_mutex_lock(&mutex);
    auto it = cache->map->find(qp.getMethodLine());
    pthread_mutex_unlock(&mutex);

    // Can't find it in the cache
    if(it == cache->map->end()){

        pthread_mutex_lock(&mutex);
        log << clientId << ": " << "not in cache" << endl;
        pthread_mutex_unlock(&mutex);

        // cout << buffer.data() << endl;

        cout << "Request not found in cache, try caching..." << endl;
        sendToServer(qp, client_connection_fd, buffer, num_bytes, cache, server_fd, clientId);
        return NULL;
    } 
    // Find it in the cache
    else {
        cout << "Request found in cache, validating..." << endl;
        // e.g: CacheControl: max-age=1000 must-revalidate
        CacheInfo* ci = it->second;
        string cacheControl = ci->getCacheControl();
        unordered_set<string> cacheControlSet;
        stringstream ss(cacheControl);
        string tmp;
        while (getline(ss, tmp, ' ')) {
            cacheControlSet.insert(tmp);
        }
        // No check time, directly revalidate
        if (cacheControl == "" || cacheControlSet.count("no-cache") != 0 || cacheControlSet.count("must-revalidate") != 0) {

            pthread_mutex_lock(&mutex);
            log << clientId << ": " << "in cache, requires validation" << endl;
            pthread_mutex_unlock(&mutex);

            cout << "Sending cached response to server, revalidating..." << endl;
            revalidate(ci, qp, client_connection_fd, buffer, num_bytes, cache, server_fd, clientId);
        }
        // Compare Expiration time with current time then decide revalidate
        else if (cacheControl.find("max-age") != string::npos) {
            time_t expirationDate = ci->getExpirationDate();
            cout << "Checking expiration date..." << endl;
            cout << "Cached response's expiration date: " << Proxy::getStringFromTime(expirationDate);
            cout << "Current date:                      " << Proxy::getStringFromTime(Proxy::getCurrentTime());
            // If cache info expires
            if (Proxy::isExpire(expirationDate)) {

                pthread_mutex_lock(&mutex);
                log << clientId << ": " << "in cache, but expired at " << getStringFromTime(expirationDate);
                pthread_mutex_unlock(&mutex);

                cout << "Cached response expired, sending cached response to server, revalidating..." << endl;
                revalidate(ci, qp, client_connection_fd, buffer, num_bytes, cache, server_fd, clientId);
            }
            // If cache info not expire
            else {

                pthread_mutex_lock(&mutex);
                log << clientId << ": " << "in cache, valid" << endl;
                pthread_mutex_unlock(&mutex);

                cout << "Cached response not expired, sending cached response to client..." << endl;

                pthread_mutex_lock(&mutex);
                string respMessage = (*(cache->map))[qp.getMethodLine()]->responseMessage;
                pthread_mutex_unlock(&mutex);

                pthread_mutex_lock(&mutex);
                stringstream ssRespMsg(respMessage);
                string sRespMsg;
                getline(ssRespMsg, sRespMsg);
                log << clientId << ": " << "Responding \"" << checkNullStr(sRespMsg).substr(0, checkNullStr(sRespMsg).length() - 1) << "\"" << endl;
                pthread_mutex_unlock(&mutex);

                int sendRes = send(client_connection_fd, respMessage.c_str(), respMessage.length(), 0);
                if (sendRes == -1) {
                    throw "Send fails";
                }
                // cout << "Success: Message sent to client (in-cache message and not expire)" << endl;
            }
        }
        // No cache
        else if (cacheControlSet.count("no-store") != 0 || cacheControlSet.count("private") != 0) {

            pthread_mutex_lock(&mutex);
            log << clientId << ": " << "not in cache" << endl;
            pthread_mutex_unlock(&mutex);

            cout << "Bypassing cache, sending request to server..." << endl;
            string s = qp.getHostname();
            
            int sendRes1 = send(server_fd, &buffer.data()[0], num_bytes, 0);
            if (sendRes1 == -1) {
                throw "Send fails";
            }
            // cout << "Success: Message sent to server" << endl;

            // GET RESPONSE FROM SERVER
            vector<char> responseBuffer (1024 * 1024);
            int response_num_bytes = recv(server_fd, &responseBuffer.data()[0], 1000 * 1000, 0);
            if (response_num_bytes < 0) {
                checkCorrupt(clientId, client_connection_fd);
                throw "Recv fails";
            }
            // cout << "Success: Message received from server" << endl;
            responseBuffer.data()[response_num_bytes] = '\0';

            // PARSE RESPONSE
            ResponseParser rp;
            rp.parse(responseBuffer);

            int contentLength = Proxy::tryGetContentLength(string(responseBuffer.begin(), responseBuffer.begin() + response_num_bytes), response_num_bytes);
            int totalLen = 0;
            int len = 0;

            string valMsg(responseBuffer.begin(), responseBuffer.begin() + response_num_bytes);

            if (contentLength >= 0) {
                while (totalLen < contentLength) {
                    // cout << "hello contentlength" << endl;
                    char nextMsg[1024];
                    len = recv(server_fd, nextMsg, 1000, 0);
                    if (len <= 0) {
                        if (totalLen >= contentLength) {
                            break;
                        }
                        else {
                            checkCorrupt(clientId, client_connection_fd);
                            throw "Recv fails";
                        }
                    }
                    string temp(nextMsg, len);
                    valMsg += temp;
                    totalLen += len;
                }
            }
            // PRINT LOG...
            // TODO
            
            // pthread_mutex_lock(&mutex);
            // log << clientId << ": " << "Responding \"" << checkNullStr(rp.getResponseLine()).substr(0, checkNullStr(rp.getResponseLine()).length() - 1) << "\"" << endl;
            // pthread_mutex_unlock(&mutex);

            // SEND RESPONSE BACK TO CLIENT
            // int sendRes2 = send(client_connection_fd, &responseBuffer.data()[0], response_num_bytes, 0);
            int sendRes2 = send(client_connection_fd, valMsg.c_str(), valMsg.length(), 0);
            if (sendRes2 == -1) {
                throw "Send fails";
            }

            pthread_mutex_lock(&mutex);
            log << clientId << ": " << "Responding \"" << checkNullStr(rp.getResponseLine()).substr(0, checkNullStr(rp.getResponseLine()).length() - 1) << "\"" << endl;
            pthread_mutex_unlock(&mutex);
            // cout << "Success: Message sent to client" << endl;
        }
        // All other minor cache control cases...
        else {
            
        }
    }
    cout << "<GET LOG ENDS>" << endl;
    cout << "---------------------------------------------------------------------------" << endl;
    return NULL;
}
time_t Proxy::getTimeFromString(string time) {
    tm tm_;
    time_t t_;
    char buffer [128];
    strcpy(buffer, time.c_str());
    strptime(buffer, "%a, %e %b %Y %T", &tm_);
    tm_.tm_isdst = -1;
    t_ = timegm(&tm_);
    return t_;
}
time_t Proxy::getCurrentTime() {
    time_t curr_time = time(NULL);
    return curr_time;
}
string Proxy::getStringFromTime(time_t rawTime) {
    return string(asctime(gmtime(&rawTime)));
}
int Proxy::comparetime(time_t time1,time_t time2){
    return difftime(time1,time2) > 0.0 ? 1 : -1; 
} 
bool Proxy::isExpire(time_t toCheck) {
    time_t now = Proxy::getCurrentTime();
    return Proxy::comparetime(now, toCheck) == 1 ? true : false;
}
void Proxy::revalidate(CacheInfo* ci, RequestParser &qp, int client_connection_fd, vector<char> &buffer,int num_bytes, Cache* cache, int server_fd, int clientId){
    string lastModified;
    string etag;
    //string toValidate = qp.getMethodLine() + "\r\n";
    //string toValidate(buffer.begin(), buffer.begin() + num_bytes);
    string toValidate;
    for (int i = 0; i < num_bytes; i += 1) {
        toValidate += buffer[i];
    }
    int toEnd = toValidate.find("\r\n\r\n");
    toValidate = toValidate.substr(0, toEnd + 4);
    // toValidate += "\r\n";
    // cout << "<BEGIN OF ORIGINAL REQUEST>" << endl;
    // cout << toValidate << endl;
    // cout << "<END OF ORIGINAL REQUEST>" << endl;
    //int toEnd = toValidate.find("\r\n\r\n");
    //toValidate = toValidate.substr(0, toEnd);
    if (ci->getLastModified() != "") {
        lastModified = ci->getLastModified();
        string validateLine = "If-Modified-Since:" + lastModified.append("\r\n");
        // toValidate += validateLine + "\r\n" + "HOST: ";
        toValidate = toValidate.insert(toValidate.length() - 2, validateLine);
        // toValidate += validateLine;
    }
    if (ci->getEtag() != "") {
        etag = ci->getEtag();
        string validateLine = "If-None-Match:" + etag.append("\r\n");
        // toValidate += validateLine + "\r\n" + "HOST: " + qp.getHostname();
        toValidate = toValidate.insert(toValidate.length() - 2, validateLine);
        // toValidate += validateLine;
    }
    // toValidate += "\r\n";
    // toValidate += "\r\n\r\n";
    // cout << "<BEGIN OF MODIFIED REQUEST>" << endl;
    // cout << toValidate << endl;
    // cout << "<END OF MODIFIED REQUEST>" << endl;
    if (ci->getLastModified() != "" || ci->getEtag() != "") {
        cout << "Sending modified request to server..." << endl;
        int sendRes = send(server_fd, toValidate.c_str(), toValidate.length(), 0);
        if (sendRes == -1) {
            throw "Send fails";
        }
        // send(server_fd, &buffer.data()[0], num_bytes, 0);
    } else {
        cout << "Sending original request to server..." << endl;
        int sendRes = send(server_fd, &buffer.data()[0], num_bytes, 0);
        if (sendRes == -1) {
            throw "Send fails";
        }
    }
    vector<char> validateBuffer (1024 * 1024);
    int validate_bytes = recv(server_fd, &validateBuffer.data()[0], 1000 * 1000, 0);
    if (validate_bytes < 0) {
        checkCorrupt(clientId, client_connection_fd);
        throw "Recv fails";
    }
    validateBuffer.data()[validate_bytes] = '\0';

    // cout << "<BEGIN OF REVALIDATION RESPONSE>" << endl;
    // cout << validateBuffer.data() << endl;
    // cout << "<END OF REVALIDATION RESPONSE>" << endl;
    ResponseParser vp;
    vp.parse(validateBuffer);
    string statusCode = vp.getStatusCode();
    // Not modified
    if(statusCode == "304"){
        cout << "Validation response: 304, sending cached response to client..." << endl;

        pthread_mutex_lock(&mutex);
        log << clientId << ": " << "Received \"HTTP/1.1 304 Not Modified\" from " << qp.getHostname() << endl;
        pthread_mutex_unlock(&mutex);
        
        pthread_mutex_lock(&mutex);
        string responseMessage = (*(cache->map))[qp.getMethodLine()]->responseMessage;
        pthread_mutex_unlock(&mutex);

        // pthread_mutex_lock(&mutex);
        // log << clientId << ": " << "Responding \"" << checkNullStr(vp.getResponseLine()).substr(0, checkNullStr(vp.getResponseLine()).length() - 1) << "\"" << endl;
        // pthread_mutex_unlock(&mutex);

        int sendRes = send(client_connection_fd, responseMessage.c_str(), responseMessage.length(), 0);
        if (sendRes == -1) {
            throw "Send fails";
        }

        pthread_mutex_lock(&mutex);
        log << clientId << ": " << "Responding \"HTTP/1.1 200 OK\"" << endl;
        pthread_mutex_unlock(&mutex);
        // cout << "Success: Message sent to client (304)" << endl;
    }
    // Modified
    else if(statusCode == "200"){

        // pthread_mutex_lock(&mutex);
        // log << clientId << ": " << "HTTP/1.1 200 OK" << endl;
        // pthread_mutex_unlock(&mutex);

        cout << "Validation response: 200, re-fetching response and sending it back to client..." << endl;
        CacheInfo* vc = new CacheInfo();
        if(vp.getLastModified() != "") {
            vc->lastModified = vp.getLastModified();
            cout << "Found header 'Last-Modified': " << vc->lastModified << endl;
        }
        if(vp.getEtag() != "") {
            vc->etag = vp.getEtag();
            cout << "Found header 'E-tag':         " << vc->etag << endl;
        }
        if(vp.getCacheControl() != "") {
            vc->cacheControl = vp.getCacheControl();
            cout << "Found header 'Cache-Control': " << vc->cacheControl << endl;
            // find max-age
            size_t vcIndex = vc->cacheControl.find("max-age");
            // if found max-age
            if (vcIndex != string::npos){
                int vcAge = atoi((vc->cacheControl.substr(vcIndex + 9, vc->cacheControl.length())).c_str());
                cout << "Found 'max-age': " << vcAge << endl;
                time_t vcExpirationDate = Proxy::getCurrentTime() + vcAge;
                vc->expirationDate = vcExpirationDate;
            }
            // if max-age not found, nothing to do...
        }

        int contentLength = Proxy::tryGetContentLength(string(validateBuffer.begin(), validateBuffer.begin() + validate_bytes), validate_bytes);
        int totalLen = 0;
        int len = 0;

        string valMsg(validateBuffer.begin(), validateBuffer.begin() + validate_bytes);

        if (contentLength >= 0) {
            while (totalLen < contentLength) {
                // cout << "hello contentlength" << endl;
                char nextMsg[1024];
                len = recv(server_fd, nextMsg, 1000, 0);
                if (len <= 0) {
                    if (totalLen >= contentLength) {
                        break;
                    }
                    else {
                        checkCorrupt(clientId, client_connection_fd);
                        throw "Recv fails";
                    }
                }
                string temp(nextMsg, len);
                valMsg += temp;
                totalLen += len;
            }
        }

        // pthread_mutex_lock(&mutex);
        // log << clientId << ": " << "Responding \"" << checkNullStr(vp.getResponseLine()).substr(0, checkNullStr(vp.getResponseLine()).length() - 1) << "\"" << endl;
        // pthread_mutex_unlock(&mutex);

        int sendRes = send(client_connection_fd, valMsg.c_str(), valMsg.length(), 0);
        if (sendRes == -1) {
            throw "Send fails";
        }

        pthread_mutex_lock(&mutex);
        log << clientId << ": " << "Responding \"" << checkNullStr(vp.getResponseLine()).substr(0, checkNullStr(vp.getResponseLine()).length() - 1) << "\"" << endl;
        pthread_mutex_unlock(&mutex);

        // string valMsg (validateBuffer.begin(), validateBuffer.begin() + validate_bytes);
        vc->responseMessage = valMsg;
        if (vc->cacheControl.find("private") != string::npos || vc->cacheControl.find("no-store") != string::npos) {
            // do nothing in cache
        }
        else {

            pthread_mutex_lock(&mutex);
            // (*(cache->map))[qp.getMethodLine()] = vc;
            cache->addToMap(qp.getMethodLine(), vc);
            pthread_mutex_unlock(&mutex);

            // cout << "Success: Message sent to client (200)" << endl;
        }
        // send(client_connection_fd, valMsg.c_str(), valMsg.length(), 0);
    }
    // Others...
    else{
        cout << "Validation response: UNKNOWN, sending it back to client..." << endl;
        string valMsg (validateBuffer.begin(), validateBuffer.begin() + validate_bytes);

        pthread_mutex_lock(&mutex);
        log << clientId << ": " << "Responding \"" << checkNullStr(vp.getResponseLine()).substr(0, checkNullStr(vp.getResponseLine()).length() - 1) << "\"" << endl;
        pthread_mutex_unlock(&mutex);

        int sendRes = send(client_connection_fd, valMsg.c_str(), valMsg.length(), 0);
        if (sendRes == -1) {
            throw "Send fails";
        }
        // cout << "Warning: Message sent to client (unkown)" << endl;
    }
}

void Proxy::sendToServer(RequestParser &qp, int client_connection_fd, vector<char> &buffer, int num_bytes, Cache* cache, int server_fd, int clientId){
    cout << "First time try sending request to server..." << endl;
    // cout << buffer.data() << endl;

    pthread_mutex_lock(&mutex);
    log << clientId << ": " << "Requesting \"" << qp.getMethodLine() << "\" from " << qp.getHostname() << endl;
    pthread_mutex_unlock(&mutex);

    int sendRes = send(server_fd, &buffer.data()[0], num_bytes, 0);
    if (sendRes == -1) {
        throw "Send fails";
    }
    cout << "First time sent request to server..." << endl;

    // GET RESPONSE FROM SERVER
    vector<char> responseBuffer (1024 * 1024);
    cout << "First time try receiving response from server..." << endl;
    int response_num_bytes = recv(server_fd, &responseBuffer.data()[0], 1000 * 1000, 0);
    if (response_num_bytes < 0) {
        checkCorrupt(clientId, client_connection_fd);
        throw "Recv fails";
    }
    cout << "First time received response from server..." << endl;
    responseBuffer.data()[response_num_bytes] = '\0';
    // cout << "Num of bytes received in the initial response: " << response_num_bytes << endl;

    // PARSE RESPONSE
    ResponseParser rp;
    rp.parse(responseBuffer);

    pthread_mutex_lock(&mutex);
    log << clientId << ": " << "Received \"" << checkNullStr(rp.getResponseLine()).substr(0, checkNullStr(rp.getResponseLine()).length() - 1) << "\" from " << qp.getHostname() << endl;
    pthread_mutex_unlock(&mutex);

    // cout << "The Content Length actaully received: " << response_num_bytes << endl;
    int contentLength = Proxy::tryGetContentLength(string(responseBuffer.begin(), responseBuffer.begin() + response_num_bytes), response_num_bytes);
    // cout << "The Content Length from Server: " << contentLength << endl;

    // Caching
    string statusCode = rp.getStatusCode();
    // Not modified
    if(statusCode == "200") {
        
        cout << "Response: 200, try caching response to cache..." << endl;
        // CHECK IF CHUNK
        if (Proxy::isChunked(&responseBuffer.data()[0], response_num_bytes)) {

            pthread_mutex_lock(&mutex);
            log << clientId << ": " << "not cacheable because it is a chunked response" << endl;
            pthread_mutex_unlock(&mutex);

            cout << "Found keyword 'Chunk', bypassing caching..." << endl;
            int sendRes = send(client_connection_fd, &responseBuffer.data()[0], response_num_bytes, 0);
            if (sendRes == -1) {
                throw "Send fails";
            }
            char chunkedMsg[28000] = {0};
            while (1) {
                // cout << "hello chunk" << endl;
                int numBytes = recv(server_fd, chunkedMsg, sizeof(chunkedMsg), 0);
                if (numBytes <= 0) {
                    
                    pthread_mutex_lock(&mutex);
                    log << clientId << ": " << "Responding \"" << checkNullStr(rp.getResponseLine()).substr(0, checkNullStr(rp.getResponseLine()).length() - 1) << "\"" << endl;
                    pthread_mutex_unlock(&mutex);

                    // cout << "End of chunk..." << endl;
                    return;
                }
                chunkedMsg[numBytes] = '\0';
                int sendRes = send(client_connection_fd, chunkedMsg, numBytes, 0);
                if (sendRes == -1) {
                    throw "Send fails";
                }
            }
        }
        int totalLen = 0;
        int len = 0;
        string msg(responseBuffer.begin(), responseBuffer.begin() + response_num_bytes);
        if (contentLength == -1) {

            pthread_mutex_lock(&mutex);
            log << clientId << ": " << "Responding \"" << checkNullStr(rp.getResponseLine()).substr(0, checkNullStr(rp.getResponseLine()).length() - 1) << "\"" << endl;
            pthread_mutex_unlock(&mutex);

            int sendRes = send(client_connection_fd, msg.c_str(), msg.length(), 0);
            if (sendRes == -1) {
                throw "Send fails";
            }
        }
        else {
            while (totalLen < contentLength) {
                // cout << "hello contentlength" << endl;
                char nextMsg[1024];
                len = recv(server_fd, nextMsg, 1000, 0);
                if (len <= 0) {
                    if (totalLen >= contentLength) {
                        break;
                    }
                    else {
                        checkCorrupt(clientId, client_connection_fd);
                        throw "Recv fails";
                    }
                }
                string temp(nextMsg, len);
                msg += temp;
                totalLen += len;
            }

            pthread_mutex_lock(&mutex);
            log << clientId << ": " << "Responding \"" << checkNullStr(rp.getResponseLine()).substr(0, checkNullStr(rp.getResponseLine()).length() - 1) << "\"" << endl;
            pthread_mutex_unlock(&mutex);

            int sendRes = send(client_connection_fd, msg.c_str(), msg.length(), 0);
            if (sendRes == -1) {
                throw "Send fails";
            }
            // cout << "<<<<<<<<<<At the end of chunk, we have: " << msg.length() << " bytes data>>>>>>>>>>" << endl;
            // cout << msg << endl;
            // cout << "<<<<<<<<<< END OF MESSAGE >>>>>>>>>>" << endl;
        }
        CacheInfo* c = new CacheInfo();
        if(rp.getLastModified() != "") {
            c->lastModified = rp.getLastModified();
            cout << "Found header 'Last-Modified': " << c->lastModified << endl;
        }
        if(rp.getEtag() != "") {
            c->etag = rp.getEtag();
            cout << "Found header 'E-tag':         " << c->etag << endl;
        }
        if(rp.getCacheControl() != ""){
            c->cacheControl = rp.getCacheControl();
            cout << "Found header 'Cache-Control': " << c->cacheControl << endl;

            if (c->cacheControl.find("no-store") != string::npos) {

                pthread_mutex_lock(&mutex);
                log << clientId << ": " << "not cacheable because it is a no-store response" << endl;
                pthread_mutex_unlock(&mutex);

                return;
            }
            if (c->cacheControl.find("private") != string::npos) {

                pthread_mutex_lock(&mutex);
                log << clientId << ": " << "not cacheable because it is a private response" << endl;
                pthread_mutex_unlock(&mutex);

                return;
            }
            if (c->cacheControl.find("no-cache" || c->cacheControl.find("must-revalidate")) != string::npos || c->cacheControl == "") {

                pthread_mutex_lock(&mutex);
                log << clientId << ": " << "cached, but requires re-validation" << endl;
                pthread_mutex_unlock(&mutex);

            }
            else {
                // find max-age
                size_t index = c->cacheControl.find("max-age");
                // if found max-age
                if (index != string::npos){
                    int age = atoi((c->cacheControl.substr(index + 8, c->cacheControl.length())).c_str());
                    cout << "Found 'max-age': " << age << endl;
                    time_t expirationDate = Proxy::getCurrentTime() + age;
                    c->expirationDate = expirationDate;

                    pthread_mutex_lock(&mutex);
                    log << clientId << ": " << "cached, expires at " << Proxy::getStringFromTime(expirationDate);
                    pthread_mutex_unlock(&mutex);

                }
            }
            // if max-age not found, nothing to do...
        }
        c->responseMessage = msg;

        pthread_mutex_lock(&mutex);
        // (*(cache->map))[qp.getMethodLine()] = c;
        cache->addToMap(qp.getMethodLine(), c);
        pthread_mutex_unlock(&mutex);

        cout << "Response: 200 OK, caching success, sending server response back to client..." << endl;
    }
    else {
        cout << "Response: " << statusCode << ", caching fails, sending server response back to client..." << endl;

        pthread_mutex_lock(&mutex);
        log << clientId << ": " << "Responding \"" << checkNullStr(rp.getResponseLine()).substr(0, checkNullStr(rp.getResponseLine()).length() - 1) << "\"" << endl;
        pthread_mutex_unlock(&mutex);

        int sendRes = send(client_connection_fd, &responseBuffer.data()[0], response_num_bytes, 0);
        if (sendRes == -1) {
            throw "Send fails";
        }
    }
    // PRINT LOG...
    // TODO

    // SEND RESPONSE BACK TO CLIENT
    // send(client_connection_fd, &responseBuffer.data()[0], response_num_bytes, 0);
    // cout << "Success: Message sent to client" << endl;
}
void* Proxy::handlePOST(int client_connection_fd, vector<char> buffer, RequestParser qp, int num_bytes, int clientId, int server_fd) {
    cout << "Client " << clientId << ": " << "Handling: " << qp.getMethodLine() << endl;
    // SEND REQUEST TO SERVER
    int sendRes2 = send(server_fd, &buffer.data()[0], num_bytes, 0);
    if (sendRes2 == -1) {
        throw "Send fails";
    }

    // GET RESPONSE FROM SERVER
    vector<char> responseBuffer (1024 * 1024);
    int response_num_bytes = recv(server_fd, &responseBuffer.data()[0], 1000 * 1000, 0);
    if (response_num_bytes <= 0) {
        checkCorrupt(clientId, client_connection_fd);
        throw "Recv fails";
    } 
    responseBuffer.data()[response_num_bytes] = '\0';

    // for (auto c: responseBuffer) cout << c;
    // cout << endl;

    // PARSE RESPONSE
    ResponseParser rp;
    rp.parse(responseBuffer);

    int contentLength = Proxy::tryGetContentLength(string(responseBuffer.begin(), responseBuffer.begin() + response_num_bytes), response_num_bytes);
    int totalLen = 0;
    int len = 0;
    string msg(responseBuffer.begin(), responseBuffer.begin() + response_num_bytes);

    if (contentLength >= 0) {
        while (totalLen < contentLength) {
            // cout << "hello contentlength" << endl;
            char nextMsg[1024];
            len = recv(server_fd, nextMsg, 1000, 0);
            if (len <= 0) {
                if (totalLen >= contentLength) {
                    break;
                }
                else {
                    checkCorrupt(clientId, client_connection_fd);
                    throw "Recv fails";
                }
            }
            string temp(nextMsg, len);
            msg += temp;
            totalLen += len;
        }
    }

    // PRINT LOG...
    // TODO

    // SEND RESPONSE BACK TO CLIENT

    pthread_mutex_lock(&mutex);
    log << clientId << ": " << "Responding \"" << checkNullStr(rp.getResponseLine()).substr(0, checkNullStr(rp.getResponseLine()).length() - 1) << "\"" << endl;
    pthread_mutex_unlock(&mutex);

    int sendRes1 = send(client_connection_fd, msg.c_str(), msg.length(), 0);
    if (sendRes1 == -1) {
        throw "Send fails";
    }
    
    // send(client_connection_fd, &responseBuffer.data()[0], response_num_bytes, 0);
    return NULL;
}
void * Proxy::handleCONNECT(int client_connection_fd, RequestParser qp, int clientId, int server_fd) {

    cout << "Client " << clientId << ": " << "Handling: " << qp.getMethodLine() << endl;
    // Handler
    int sendRes = send(client_connection_fd, "HTTP/1.1 200 OK\r\n\r\n", 19, 0);
    if (sendRes == -1) {
        throw "Send fails";
    }

    pthread_mutex_lock(&mutex);
    log << clientId << ": " << "Responding \"HTTP/1.1 200 OK\"" << endl;
    pthread_mutex_unlock(&mutex);

    vector<int> clientServer;
    clientServer.push_back(client_connection_fd);
    clientServer.push_back(server_fd);
    fd_set fd_set;
    int nfds = -1;
    nfds = max(max(client_connection_fd, server_fd), nfds) + 1;
    while (1) {
        FD_ZERO(&fd_set);
        FD_SET(clientServer[0], &fd_set);
        FD_SET(clientServer[1], &fd_set);
        int sr = select(nfds, &fd_set, NULL, NULL, NULL);
        if (sr >= 0) {
            for (int i = 0; i < 2; i += 1) {
                if (FD_ISSET(clientServer[i], &fd_set)) {
                    vector<char> connectRequestBuffer (1024 * 1024);
                    int connect_request_num_bytes = recv(clientServer[i], &connectRequestBuffer.data()[0], 1000 * 1000, 0);
                    if (connect_request_num_bytes <= 0) {

                        pthread_mutex_lock(&mutex);
                        log << clientId << ": " << "Tunnel closed" << endl;
                        pthread_mutex_unlock(&mutex);

                        return NULL;
                    }
                    connectRequestBuffer.data()[connect_request_num_bytes] = '\0';
                    if (connect_request_num_bytes <= 0) {
                        // cout << "CONNECT-Exit..." << endl;

                        pthread_mutex_lock(&mutex);
                        log << clientId << ": " << "Tunnel closed" << endl;
                        pthread_mutex_unlock(&mutex);

                        return NULL;
                    }
                    if (send(clientServer[1 - i], &connectRequestBuffer.data()[0], connect_request_num_bytes, 0) <= 0) {
                        // cout << "CONNECT-Exit..." << endl;

                        pthread_mutex_lock(&mutex);
                        log << clientId << ": " << "Tunnel closed" << endl;
                        pthread_mutex_unlock(&mutex);

                        return NULL;
                    }
                }
            }
        } else {
            // cout << "CONNECT-Exit..." << endl;

            pthread_mutex_lock(&mutex);
            log << clientId << ": " << "Tunnel closed" << endl;
            pthread_mutex_unlock(&mutex);

            return NULL;
        }
    }
}