#include <string>
#include <time.h>

using namespace std;

class CacheInfo{

    public:
        string lastModified;
        string etag;
        string cacheControl;
        string responseMessage;
        time_t expirationDate;

        CacheInfo();
        string getLastModified();
        string getEtag();
        string getCacheControl();
        string getResponseMessage();
        time_t getExpirationDate();

};