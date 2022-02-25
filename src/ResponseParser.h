#include <vector>
#include <string>

using namespace std;

class ResponseParser {
    private:
        string statusCode;
        string responseLine;
        string contentLength; // not sure
        string etag; // optional 
        string lastModified; // optional
        string cacheControl; // optional

    public:
        ResponseParser();
        void parse(vector<char>& buffer);
        string getStatusCode();
        string getResponseLine();
        string getContentLength();
        string getEtag();
        string getLastModified();
        string getCacheControl();
        static string getHeader(string line);
        static string getContent(string line);

};