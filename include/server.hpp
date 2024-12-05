#ifndef server_h
#define server_h

#include <node.hpp>
#include <portlock.hpp>
#include <memory>
#include <string>

class Server : public Node {
public:
    Server(string& host, int port);
    void SetResponseBuffer(string buffer);
    void run() override;
    ~Server();
private:
    std::string response_buffer;
    std::unique_ptr<PortLock> port_lock;
};

#endif