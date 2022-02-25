#include <string>
#include <unordered_map>
#include <queue>
#include "CacheInfo.h"
#include<iostream>

using namespace std;

class Cache {

    public:
        unordered_map<string, CacheInfo*>* map = new unordered_map< string, CacheInfo * >();
        queue <string>* q = new queue<string>();
        int size = 0;
        int capacity = 10;

    public:
        ~Cache();
        void addToMap(string key, CacheInfo* value);

};