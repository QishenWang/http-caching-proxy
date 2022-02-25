#include "CacheInfo.h"

using namespace std;

CacheInfo::CacheInfo(){
    this->lastModified = "";
    this->etag = "";
    this->cacheControl = "";
    this->responseMessage = "";
    this->expirationDate = 0;
}
string CacheInfo::getLastModified(){
    return this->lastModified;
}

string CacheInfo::getEtag(){
    return this->etag;
}

string CacheInfo::getCacheControl(){
    return this->cacheControl;
}

string CacheInfo::getResponseMessage(){
    return this->responseMessage;
}

time_t CacheInfo::getExpirationDate() {
    return this->expirationDate;
}