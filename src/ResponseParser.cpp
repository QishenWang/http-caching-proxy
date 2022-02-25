#include "ResponseParser.h"
#include <vector>
#include <sstream>
#include <iostream>

using namespace std;

ResponseParser::ResponseParser() {
    string statusCode = "";
    string responseLine = "";
    string contentLength = ""; // not sure
    string etag = ""; // optional 
    string lastModified = ""; // optional
    string cacheControl = ""; // optional
}
void ResponseParser::parse(vector<char>& buffer) {
    int i = 0;
    string message, tmp;
    while(buffer.data()[i]!= 0 && i < (int) buffer.size()){
        message += buffer.data()[i];
        i += 1;
    }
    if(i == (int) buffer.size()){
        // Error
        cout << "Parser-Error: no null terminator" << endl;
    }
    stringstream ss(message);
    getline(ss, this->responseLine);
    stringstream rs(this->responseLine);
    getline(rs, this->statusCode, ' ');
    getline(rs, this->statusCode, ' ');

    while(getline(ss, tmp)){
        string header = this->getHeader(tmp);
        if (header == "Last-Modified") {
            this->lastModified = this->getContent(tmp);
        }
        if (header == "ETag") {
            this->etag = this->getContent(tmp);
        }
        if (header == "Content-Length") {
            this->contentLength = this->getContent(tmp);
        }
        if (header == "Cache-Control") {
            this->cacheControl = this->getContent(tmp);
        }
    }
}
string ResponseParser::getStatusCode() {
    return this->statusCode;
}
string ResponseParser::getResponseLine() {
    return this->responseLine;
}
string ResponseParser::getContentLength() {
    return this->contentLength;
}
string ResponseParser::getEtag() {
    return this->etag;
}
string ResponseParser::getLastModified() {
    return this->lastModified;
}
string ResponseParser::getCacheControl() {
    return this->cacheControl;
}
string ResponseParser::getHeader(string line) {
    int i = 0;
    while(line[i] != ':' && i < (int) line.size()){
        i++;
    } 
    return line.substr(0,i);
}
string ResponseParser::getContent(string line) {
    int i = 0;
    while(line[i] != ':' && i < (int) line.length()){
        i++;
    } 
    i++;
    while (line[i] == ' ' && i < (int) line.length()) {
        i++;
    }
    size_t endR = line.find("\r");
    if (endR != string::npos) {
        return line.substr(i, endR - i);
    }
    size_t endN = line.find("\n");
    if (endN != string::npos) {
        return line.substr(i, endN - i);
    }
    return line.substr(i, line.length() - i);
}