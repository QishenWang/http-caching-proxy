#include <vector>
#include <string>

using namespace std;

class RequestParser {
    private:
        string method; 
        string methodLine;
        string host;
        string hostLine;

    public:
        void parse(vector<char>& buffer);
        string getMethod();
        string getHostname();
        string getMethodLine();

};