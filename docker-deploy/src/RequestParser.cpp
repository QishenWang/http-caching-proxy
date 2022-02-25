#include "RequestParser.h"
#include <vector>
#include <sstream>
#include <iostream>
#include <string>

using namespace std;


void RequestParser::parse(vector<char>& buffer){
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
    getline(ss, this->methodLine);
    // getline(ss, this->hostLine);
    this->methodLine = this->methodLine.substr(0, this->methodLine.length()-1);
    
    stringstream ms(this->methodLine);
    getline(ms, this->method, ' ');

    stringstream hs(message);
    string hostTmp;
    while (getline(ss, hostTmp, '\n')) {
        size_t idx = hostTmp.find("Host: ");
        if (idx != string::npos) {
            this->hostLine = hostTmp.substr(0, hostTmp.length() - 1);
            size_t endIdx = hostTmp.find('\r');
            this->host = hostTmp.substr(6, endIdx - 6);
        }
    }
}
string RequestParser::getMethod() {
    return this->method;
}
string RequestParser::getHostname() {
    return this->host;
}
string RequestParser::getMethodLine() {
    return this->methodLine;
}