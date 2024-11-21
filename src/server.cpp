#include <server.hpp>
#include <iostream>

#define MAXLINE 1024

Server::Server(string& host, int port) : Node(host, port) {}

void Server::SetResponseBuffer(string buffer) {
    this->response_buffer = buffer;
}

void Server::handleMessage(void *buffer) {
    sockaddr_in cliaddr;
    socklen_t len = sizeof(cliaddr);
    auto n = connection.recv(buffer, MAXLINE, &cliaddr, &len);
    ((char*)buffer)[n] = '\0';
    cout << "Message from client: " << (char*)buffer << endl;
    connection.send(inet_ntoa(cliaddr.sin_addr), cliaddr.sin_port, (void*)response_buffer.c_str(), response_buffer.size());
}

Server::~Server() {
    connection.close();
}
