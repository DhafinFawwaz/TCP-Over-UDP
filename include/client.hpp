#ifndef client_h
#define client_h

#include <node.hpp>

class Client : public Node
{
private:
    char* host;
    char* port;
public:
    Client(char* host, char* port);
    void handleMessage(void *buffer) override;
};

#endif