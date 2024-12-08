#ifndef server_h
#define server_h

#include <node.hpp>
#include <portlock.hpp>
#include <memory>
#include <string>

class Server : public Node
{
public:
    Server(string& host, int port);
    void SetResponseBuffer(vector<char>& buffer);
    void run() override;
    ~Server();
private:
    vector<char> response_buffer;
    std::unique_ptr<PortLock> port_lock;
};

#endif