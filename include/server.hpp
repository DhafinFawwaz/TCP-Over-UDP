#ifndef server_h
#define server_h

#include <node.hpp>

class Server : public Node
{
    vector<char> response_buffer;
public:
    Server(string& host, int port);
    void SetResponseBuffer(vector<char>& buffer);
    void run() override;
    ~Server();
};

#endif