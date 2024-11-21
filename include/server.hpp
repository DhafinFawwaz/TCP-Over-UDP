#ifndef server_h
#define server_h

#include <node.hpp>

class Server : public Node
{
private:
    string host;
    string port;
public:
    Server(string host, string port);
    void handleMessage(void *buffer) override;
};

#endif