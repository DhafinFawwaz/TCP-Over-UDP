#ifndef client_h
#define client_h

#include <node.hpp>

class Client : public Node
{
private:
    string server_ip;
    int server_port;
public:
    Client(string& host, int port);
    void setServerTarget(string server_ip, int server_port);
    void handleMessage(void *buffer) override;
    ~Client();
};

#endif