#include <node.hpp>
#include <iostream>
#include <unistd.h>

Node::Node(string& host, int port) : host(host), port(port), connection((char*)host.c_str(), port) {}

void Node::run() {
    connection.listen();
    while(true) {
        char buffer[1024];
        handleMessage(buffer);
        sleep(1);
    }
}