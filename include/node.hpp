#ifndef node_h
#define node_h

#include <socket.hpp>

/**
 * Abstract class.
 *
 * This is the base class for Server and Client class.
 */
class Node
{
protected:
    string host;
    int port;
    TCPSocket connection;

public:
    Node(string& host, int port);
    void run();
    virtual void handleMessage(void *buffer) = 0;
};

#endif