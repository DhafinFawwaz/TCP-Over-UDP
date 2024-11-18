#ifndef server_h
#define server_h

#include <node.hpp>

class Server : public Node
{
private:
    char* host;
    char* port;
public:
    Server(char* host, char* port);
    void handleMessage(void *buffer) override;
};

#endif