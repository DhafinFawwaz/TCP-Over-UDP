#ifndef client_h
#define client_h

#include <node.hpp>

class Client : public Node
{
private:
    string host;
    string port;
public:
    Client(string host, string port);
    void handleMessage(void *buffer) override;
};

#endif