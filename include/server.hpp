#ifndef server_h
#define server_h

#include <node.hpp>

class Server : public Node
{
    string response_buffer;
public:
    Server(string& host, int port);
    void SetResponseBuffer(string buffer);
    void run() override;
    ~Server();
};

#endif