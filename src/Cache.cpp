#include "Cache.h"

using namespace std;

Cache::~Cache() {
    for(unordered_map<string, CacheInfo*>::iterator itr = this->map->begin(); itr != this->map->end(); itr++)
    {
        delete (itr->second);
    }
    this->map->clear();
} 
void Cache::addToMap(string key, CacheInfo* value) {
    if (this->size >= this->capacity) {
        string toRemove = this->q->front();
        cout << "CACHE LOG: size exceeds maximum capacity, removing " << toRemove << "..." << endl;
        this->q->pop();
        this->map->erase(toRemove);
        (this->size)--;
    }
    this->q->push(key);
    (*(this->map))[key] = value;
    (this->size)++;
    cout << "CACHE LOG: current size is " << this->size << endl;
}